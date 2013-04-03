# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import os
from mozpack.errors import errors
from mozpack.files import (
    BaseFile,
    Dest,
)
import mozpack.path
import errno
from collections import OrderedDict


def ensure_parent_dir(file):
    '''Ensures the directory parent to the given file exists'''
    dir = os.path.dirname(file)
    if not dir or os.path.exists(dir):
        return
    try:
        os.makedirs(dir)
    except OSError as error:
        if error.errno != errno.EEXIST:
            raise


class FileRegistry(object):
    '''
    Generic container to keep track of a set of BaseFile instances. It
    preserves the order under which the files are added, but doesn't keep
    track of empty directories (directories are not stored at all).
    The paths associated with the BaseFile instances are relative to an
    unspecified (virtual) root directory.

        registry = FileRegistry()
        registry.add('foo/bar', file_instance)
    '''

    def __init__(self):
        self._files = OrderedDict()

    def add(self, path, content):
        '''
        Add a BaseFile instance to the container, under the given path.
        '''
        assert isinstance(content, BaseFile)
        if path in self._files:
            return errors.error("%s already added" % path)
        # Check whether any parent of the given path is already stored
        partial_path = path
        while partial_path:
            partial_path = mozpack.path.dirname(partial_path)
            if partial_path in self._files:
                return errors.error("Can't add %s: %s is a file" %
                                    (path, partial_path))
        self._files[path] = content

    def match(self, pattern):
        '''
        Return the list of paths, stored in the container, matching the
        given pattern. See the mozpack.path.match documentation for a
        description of the handled patterns.
        '''
        if '*' in pattern:
            return [p for p in self.paths()
                    if mozpack.path.match(p, pattern)]
        if pattern == '':
            return self.paths()
        if pattern in self._files:
            return [pattern]
        return [p for p in self.paths()
                if mozpack.path.basedir(p, [pattern]) == pattern]

    def remove(self, pattern):
        '''
        Remove paths matching the given pattern from the container. See the
        mozpack.path.match documentation for a description of the handled
        patterns.
        '''
        items = self.match(pattern)
        if not items:
            return errors.error("Can't remove %s: %s" % (pattern,
                                "not matching anything previously added"))
        for i in items:
            del self._files[i]

    def paths(self):
        '''
        Return all paths stored in the container, in the order they were added.
        '''
        return self._files.keys()

    def __len__(self):
        '''
        Return number of paths stored in the container.
        '''
        return len(self._files)

    def __contains__(self, pattern):
        raise RuntimeError("'in' operator forbidden for %s. Use contains()." %
                           self.__class__.__name__)

    def contains(self, pattern):
        '''
        Return whether the container contains paths matching the given
        pattern. See the mozpack.path.match documentation for a description of
        the handled patterns.
        '''
        return len(self.match(pattern)) > 0

    def __getitem__(self, path):
        '''
        Return the BaseFile instance stored in the container for the given
        path.
        '''
        return self._files[path]

    def __iter__(self):
        '''
        Iterate over all (path, BaseFile instance) pairs from the container.
            for path, file in registry:
                (...)
        '''
        return self._files.iteritems()


class FileCopier(FileRegistry):
    '''
    FileRegistry with the ability to copy the registered files to a separate
    directory.
    '''
    def copy(self, destination, skip_if_older=True):
        '''
        Copy all registered files to the given destination path. The given
        destination can be an existing directory, or not exist at all. It
        can't be e.g. a file.
        The copy process acts a bit like rsync: files are not copied when they
        don't need to (see mozpack.files for details on file.copy), and files
        existing in the destination directory that aren't registered are
        removed.
        '''
        assert isinstance(destination, basestring)
        assert not os.path.exists(destination) or os.path.isdir(destination)
        destination = os.path.normpath(destination)
        dest_files = set()
        for path, file in self:
            destfile = os.path.normpath(os.path.join(destination, path))
            dest_files.add(destfile)
            ensure_parent_dir(destfile)
            file.copy(destfile, skip_if_older)

        actual_dest_files = set()
        for root, dirs, files in os.walk(destination):
            for f in files:
                actual_dest_files.add(os.path.normpath(os.path.join(root, f)))
        for f in actual_dest_files - dest_files:
            os.remove(f)
        for root, dirs, files in os.walk(destination):
            if not files and not dirs:
                os.removedirs(root)


class Jarrer(FileRegistry, BaseFile):
    '''
    FileRegistry with the ability to copy and pack the registered files as a
    jar file. Also acts as a BaseFile instance, to be copied with a FileCopier.
    '''
    def __init__(self, compress=True, optimize=True):
        '''
        Create a Jarrer instance. See mozpack.mozjar.JarWriter documentation
        for details on the compress and optimize arguments.
        '''
        self.compress = compress
        self.optimize = optimize
        self._preload = []
        FileRegistry.__init__(self)

    def copy(self, dest, skip_if_older=True):
        '''
        Pack all registered files in the given destination jar. The given
        destination jar may be a path to jar file, or a Dest instance for
        a jar file.
        If the destination jar file exists, its (compressed) contents are used
        instead of the registered BaseFile instances when appropriate.
        '''
        class DeflaterDest(Dest):
            '''
            Dest-like class, reading from a file-like object initially, but
            switching to a Deflater object if written to.

                dest = DeflaterDest(original_file)
                dest.read()      # Reads original_file
                dest.write(data) # Creates a Deflater and write data there
                dest.read()      # Re-opens the Deflater and reads from it
            '''
            def __init__(self, orig=None, compress=True):
                self.mode = None
                self.deflater = orig
                self.compress = compress

            def read(self, length=-1):
                if self.mode != 'r':
                    assert self.mode is None
                    self.mode = 'r'
                return self.deflater.read(length)

            def write(self, data):
                if self.mode != 'w':
                    from mozpack.mozjar import Deflater
                    self.deflater = Deflater(self.compress)
                    self.mode = 'w'
                self.deflater.write(data)

            def exists(self):
                return self.deflater is not None

        if isinstance(dest, basestring):
            dest = Dest(dest)
        assert isinstance(dest, Dest)

        from mozpack.mozjar import JarWriter, JarReader
        try:
            old_jar = JarReader(fileobj=dest)
        except Exception:
            old_jar = []

        old_contents = dict([(f.filename, f) for f in old_jar])

        with JarWriter(fileobj=dest, compress=self.compress,
                       optimize=self.optimize) as jar:
            for path, file in self:
                if path in old_contents:
                    deflater = DeflaterDest(old_contents[path], self.compress)
                else:
                    deflater = DeflaterDest(compress=self.compress)
                file.copy(deflater, skip_if_older)
                jar.add(path, deflater.deflater)
            if self._preload:
                jar.preload(self._preload)

    def open(self):
        raise RuntimeError('unsupported')

    def preload(self, paths):
        '''
        Add the given set of paths to the list of preloaded files. See
        mozpack.mozjar.JarWriter documentation for details on jar preloading.
        '''
        self._preload.extend(paths)

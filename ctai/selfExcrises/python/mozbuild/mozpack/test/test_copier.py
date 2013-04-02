# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

from mozpack.copier import (
    FileCopier,
    FileRegistry,
    Jarrer,
)
from mozpack.files import GeneratedFile
from mozpack.mozjar import JarReader
import mozpack.path
import unittest
import mozunit
import os
import shutil
from mozpack.errors import ErrorMessage
from tempfile import mkdtemp
from mozpack.test.test_files import (
    MockDest,
    MatchTestTemplate,
)


class TestFileRegistry(MatchTestTemplate, unittest.TestCase):
    def add(self, path):
        self.registry.add(path, GeneratedFile(path))

    def do_check(self, pattern, result):
        self.checked = True
        if result:
            self.assertTrue(self.registry.contains(pattern))
        else:
            self.assertFalse(self.registry.contains(pattern))
        self.assertEqual(self.registry.match(pattern), result)

    def test_file_registry(self):
        self.registry = FileRegistry()
        self.registry.add('foo', GeneratedFile('foo'))
        bar = GeneratedFile('bar')
        self.registry.add('bar', bar)
        self.assertEqual(self.registry.paths(), ['foo', 'bar'])
        self.assertEqual(self.registry['bar'], bar)

        self.assertRaises(ErrorMessage, self.registry.add, 'foo',
                          GeneratedFile('foo2'))

        self.assertRaises(ErrorMessage, self.registry.remove, 'qux')

        self.assertRaises(ErrorMessage, self.registry.add, 'foo/bar',
                          GeneratedFile('foobar'))
        self.assertRaises(ErrorMessage, self.registry.add, 'foo/bar/baz',
                          GeneratedFile('foobar'))

        self.assertEqual(self.registry.paths(), ['foo', 'bar'])

        self.registry.remove('foo')
        self.assertEqual(self.registry.paths(), ['bar'])
        self.registry.remove('bar')
        self.assertEqual(self.registry.paths(), [])

        self.prepare_match_test()
        self.do_match_test()
        self.assertTrue(self.checked)
        self.assertEqual(self.registry.paths(), [
            'bar',
            'foo/bar',
            'foo/baz',
            'foo/qux/1',
            'foo/qux/bar',
            'foo/qux/2/test',
            'foo/qux/2/test2',
        ])

        self.registry.remove('foo/qux')
        self.assertEqual(self.registry.paths(), ['bar', 'foo/bar', 'foo/baz'])

        self.registry.add('foo/qux', GeneratedFile('fooqux'))
        self.assertEqual(self.registry.paths(), ['bar', 'foo/bar', 'foo/baz',
                                                 'foo/qux'])
        self.registry.remove('foo/b*')
        self.assertEqual(self.registry.paths(), ['bar', 'foo/qux'])

        self.assertEqual([f for f, c in self.registry], ['bar', 'foo/qux'])
        self.assertEqual(len(self.registry), 2)

        self.add('foo/.foo')
        self.assertTrue(self.registry.contains('foo/.foo'))


class TestFileCopier(unittest.TestCase):
    def setUp(self):
        self.tmpdir = mkdtemp()

    def tearDown(self):
        shutil.rmtree(self.tmpdir)

    def all_dirs(self, base):
        all_dirs = set()
        for root, dirs, files in os.walk(base):
            if not dirs:
                all_dirs.add(mozpack.path.relpath(root, base))
        return all_dirs

    def all_files(self, base):
        all_files = set()
        for root, dirs, files in os.walk(base):
            for f in files:
                all_files.add(
                    mozpack.path.join(mozpack.path.relpath(root, base), f))
        return all_files

    def test_file_copier(self):
        copier = FileCopier()
        copier.add('foo/bar', GeneratedFile('foobar'))
        copier.add('foo/qux', GeneratedFile('fooqux'))
        copier.add('foo/deep/nested/directory/file', GeneratedFile('fooz'))
        copier.add('bar', GeneratedFile('bar'))
        copier.add('qux/foo', GeneratedFile('quxfoo'))
        copier.add('qux/bar', GeneratedFile(''))

        copier.copy(self.tmpdir)
        self.assertEqual(self.all_files(self.tmpdir), set(copier.paths()))
        self.assertEqual(self.all_dirs(self.tmpdir),
                         set(['foo/deep/nested/directory', 'qux']))

        copier.remove('foo')
        copier.add('test', GeneratedFile('test'))
        copier.copy(self.tmpdir)
        self.assertEqual(self.all_files(self.tmpdir), set(copier.paths()))
        self.assertEqual(self.all_dirs(self.tmpdir), set(['qux']))


class TestJarrer(unittest.TestCase):
    def check_jar(self, dest, copier):
        jar = JarReader(fileobj=dest)
        self.assertEqual([f.filename for f in jar], copier.paths())
        for f in jar:
            self.assertEqual(f.uncompressed_data.read(),
                             copier[f.filename].content)

    def test_jarrer(self):
        copier = Jarrer()
        copier.add('foo/bar', GeneratedFile('foobar'))
        copier.add('foo/qux', GeneratedFile('fooqux'))
        copier.add('foo/deep/nested/directory/file', GeneratedFile('fooz'))
        copier.add('bar', GeneratedFile('bar'))
        copier.add('qux/foo', GeneratedFile('quxfoo'))
        copier.add('qux/bar', GeneratedFile(''))

        dest = MockDest()
        copier.copy(dest)
        self.check_jar(dest, copier)

        copier.remove('foo')
        copier.add('test', GeneratedFile('test'))
        copier.copy(dest)
        self.check_jar(dest, copier)

        copier.remove('test')
        copier.add('test', GeneratedFile('replaced-content'))
        copier.copy(dest)
        self.check_jar(dest, copier)

        copier.copy(dest)
        self.check_jar(dest, copier)

        preloaded = ['qux/bar', 'bar']
        copier.preload(preloaded)
        copier.copy(dest)

        dest.seek(0)
        jar = JarReader(fileobj=dest)
        self.assertEqual([f.filename for f in jar], preloaded +
                         [p for p in copier.paths() if not p in preloaded])
        self.assertEqual(jar.last_preloaded, preloaded[-1])

if __name__ == '__main__':
    mozunit.main()

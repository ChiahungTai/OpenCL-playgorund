=================
mozbuild.frontend
=================

The mozbuild.frontend package is of sufficient importance and complexity
to warrant its own README file. If you are looking for documentation on
how the build system gets started, you've come to the right place.

Overview
========

Tree metadata (including the build system) is defined by a collection of
files in the source tree called *mozbuild* files. These typically are files
named *moz.build*. But, the actual name can vary.

Each *mozbuild* file defines basic metadata about the part of the tree
(typically directory scope) it resides in. This includes build system
configuration, such as the list of C++ files to compile or headers to install
or libraries to link together.

*mozbuild* files are actually Python scripts. However, their execution
is governed by special rules. This will be explained later.

Once a *mozbuild* file has executed, it is converted into a set of static
data structures.

The set of all data structures from all relevant *mozbuild* files
constitute all of the metadata from the tree.

How *mozbuild* Files Work
=========================

As stated above, *mozbuild* files are actually Python scripts. However,
their behavior is very different from what you would expect if you executed
the file using the standard Python interpreter from the command line.

There are two properties that make execution of *mozbuild* files special:

1. They are evaluated in a sandbox which exposes a limited subset of Python
2. There is a special set of global variables which hold the output from
   execution.

The limited subset of Python is actually an extremely limited subset.
Only a few built-ins are exposed. These include *True*, *False*, and
*None*. Global functions like *import*, *print*, and *open* aren't defined.
Without these, *mozbuild* files can do very little. This is by design.

The side-effects of the execution of a *mozbuild* file are used to define
the build configuration. Specifically, variables set during the execution
of a *mozbuild* file are examined and their values are used to populate
data structures.

The enforced convention is that all UPPERCASE names inside a sandbox are
reserved and it is the value of these variables post-execution that is
examined. Furthermore, the set of allowed UPPERCASE variable names and
their types is statically defined. If you attempt to reference or assign
to an UPPERCASE variable name that isn't known to the build system or
attempt to assign a value of the wrong type (e.g. a string when it wants a
list), an error will be raised during execution of the *mozbuild* file.
This strictness is to ensure that assignment to all UPPERCASE variables
actually does something. If things weren't this way, *mozbuild* files
might think they were doing something but in reality wouldn't be. We don't
want to create false promises, so we validate behavior strictly.

If a variable is not UPPERCASE, you can do anything you want with it,
provided it isn't a function or other built-in. In other words, normal
Python rules apply.

All of the logic for loading and evaluating *mozbuild* files is in the
*reader* module. Of specific interest is the *MozbuildSandbox* class. The
*BuildReader* class is also important, as it is in charge of
instantiating *MozbuildSandbox* instances and traversing a tree of linked
*mozbuild* files. Unless you are a core component of the build system,
*BuildReader* is probably the only class you care about in this module.

The set of variables and functions *exported* to the sandbox is defined by
the *sandbox_symbols* module. These data structures are actually used to
populate MozbuildSandbox instances. And, there are tests to ensure that the
sandbox doesn't add new symbols without those symbols being added to the
module. And, since the module contains documentation, this ensures the
documentation is up to date (at least in terms of symbol membership).

How Sandboxes are Converted into Data Structures
================================================

The output of a *mozbuild* file execution is essentially a dict of all
the special UPPERCASE variables populated during its execution. While these
dicts are data structures, they aren't the final data structures that
represent the build configuration.

We feed the *mozbuild* execution output (actually *reader.MozbuildSandbox*
instances) into a *TreeMetadataEmitter* class instance. This class is
defined in the *emitter* module. *TreeMetadataEmitter* converts the
*MozbuildSandbox* instances into instances of the *TreeMetadata*-derived
classes from the *data* module.

All the classes in the *data* module define a domain-specific
component of the tree metadata, including build configuration. File compilation
and IDL generation are separate classes, for example. The only thing these
classes have in common is that they inherit from *TreeMetadata*, which is
merely an abstract base class.

The set of all emitted *TreeMetadata* instances (converted from executed
*mozbuild* files) constitutes the aggregate tree metadata. This is the
the authoritative definition of the build system, etc and is what's used by
all downstream consumers, such as build backends. There is no monolithic
class or data structure. Instead, the tree metadata is modeled as a collection
of *TreeMetadata* instances.

There is no defined mapping between the number of
*MozbuildSandbox*/*moz.build* instances and *TreeMetadata* instances.
Some *mozbuild* files will emit only 1 *TreeMetadata* instance. Some
will emit 7. Some may even emit 0!

The purpose of this *emitter* layer between the raw *mozbuild* execution
result and *TreeMetadata* is to facilitate additional normalization and
verification of the output. There are multiple downstream consumers of
this data and there is common functionality shared between them. An
abstraction layer that provides high-level filtering is a useful feature.
Thus *TreeMetadataEmitter* exists.

Other Notes
===========

*reader.BuildReader* and *emitter.TreeMetadataEmitter* have a nice
stream-based API courtesy of generators. When you hook them up properly,
*TreeMetadata* instances can be consumed before all *mozbuild* files have
been read. This means that errors down the pipe can trigger before all
upstream tasks (such as executing and converting) are complete. This should
reduce the turnaround time in the event of errors. This likely translates to
a more rapid pace for implementing backends, which require lots of iterative
runs through the entire system.

Lots of code in this sub-module is applicable to other systems, not just
Mozilla's. However, some of the code is tightly coupled. If there is a will
to extract the generic bits for re-use in other projects, that can and should
be done.

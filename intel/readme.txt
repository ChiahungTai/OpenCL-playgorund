===================================================
Using Intel® SDK for OpenCL* Applications Samples
===================================================

Intel® SDK for OpenCL* Applications 2012 Samples are supported only on Microsoft Windows* operating systems.

==============================
Building a Sample Application
==============================
The Intel® SDK for OpenCL* Applications 2012 sample package contains a Microsoft* Visual Studio* 2008 and 2010 solution file, *.sln, residing in:

	<SampleNameDirectory>\*.sln 

NOTE:	Microsoft* Visual Studio* 2008 solution file has a "_2008" suffix in a name
	Microsoft* Visual Studio* 2010 solution file has a "_2010" suffix in a name 

To build sample in the solution, open the solution files and select Build > Build Solution.

==============================
Running a Sample Application 
==============================

You can run a sample application using Microsoft Visual Studio* 2008 and 2010, or using the command line.

==============================
Using Microsoft Visual Studio*
==============================

1. Open and build <sample name>.sln
2. Select a project file in the Solution Explorer.
3. Right-click the project and select Set as StartUp Project. 
Press Ctrl+F5 to run the application. To run the application in debug mode, press F5.

==================
Using Command Line
==================

1. Open Microsoft Visual Studio* command prompt.
2. Change to the directory according to the platform configuration on which you want to run the executable:
	• \Win32 - for Win32 configuration
	• \x64 - for x64 configuration.
3. Open the appropriate project configuration (Debug or Release). Run the sample by entering the name of the executable.  
4. You can run the sample with command line option --h to print all possible command line options for the sample. 

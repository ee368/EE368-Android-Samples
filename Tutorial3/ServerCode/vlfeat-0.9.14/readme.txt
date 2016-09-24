MATLAB Compiler

1. Prerequisites for Deployment 

. Verify the MATLAB Compiler Runtime (MCR) is installed and ensure you    
  have installed version 7.16.   

. If the MCR is not installed, launch MCRInstaller, located in:

  <matlabroot>*/toolbox/compiler/deploy/glnxa64/MCRInstaller.zip

For more information about the MCR and the MCR Installer, see 
“Working With the MCR” in the MATLAB Compiler User’s Guide.    



2. Files to Deploy and Package

Files to package for Standalone 
================================
-ee368_demo 
-run_ee368_demo.sh (shell script run to temporarily set environment variables and execute 
                    the application)
   -to run the shell script, type
   
       ./run_ee368_demo.sh <mcr_directory> <argument_list>
       
    at Linux or Mac command prompt. <mcr_directory> is the directory 
    where version 7.16 of MCR is installed or the directory where 
    MATLAB is installed on the machine. <argument_list> is all the 
    arguments you want to pass to your application. For example, 

    If you have version 7.16 of MCR installed in 
    /mathworks/home/application/R2010a/v716, run the shell script as:
    
       ./run_ee368_demo.sh /mathworks/home/application/R2010a/v716
       
    If you have MATLAB installed in /mathworks/devel/application/matlab, 
    run the shell script as:
    
       ./run_ee368_demo.sh /mathworks/devel/application/matlab
-MCRInstaller.zip
   -include when building component by clicking "Add MCR" link 
    in deploytool
-This readme file 

3. Definitions

For information on deployment terminology, go to 
http://www.mathworks.com/help. Select your product and see 
the Glossary in the User’s Guide.


* NOTE: <matlabroot> is the directory where MATLAB is installed on the target machine.


4. Appendix 

A. Linux x86-64 systems:
   On the target machine, add the MCR directory to the environment variable 
   LD_LIBRARY_PATH by issuing the following commands:

        NOTE: <mcr_root> is the directory where MCR is installed
              on the target machine.         

            setenv LD_LIBRARY_PATH
                $LD_LIBRARY_PATH:
                <mcr_root>/v716/runtime/glnxa64:
                <mcr_root>/v716/bin/glnxa64:
                <mcr_root>/v716/sys/os/glnxa64:
                <mcr_root>/v716/sys/java/jre/glnxa64/jre/lib/amd64/native_threads:
                <mcr_root>/v716/sys/java/jre/glnxa64/jre/lib/amd64/server:
                <mcr_root>/v716/sys/java/jre/glnxa64/jre/lib/amd64 
            setenv XAPPLRESDIR <mcr_root>/v716/X11/app-defaults


     
        NOTE: To make these changes persistent after logout on Linux 
              or Mac machines, modify the .cshrc file to include this  
              setenv command.
        NOTE: The environment variable syntax utilizes forward 
              slashes (/), delimited by colons (:).  
        NOTE: When deploying standalone applications, it is possible 
              to run the shell script file run_ee368_demo.sh 
              instead of setting environment variables. See 
              section 2 "Files to Deploy and Package".    







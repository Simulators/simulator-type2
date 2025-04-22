HOW TO USE THESE KICAD CAD FILES
================================

*** If you just want to order PCBs from a fabrication house, you don't need THESE files ***
Ready-made Gerber files can be found in the Gerbers directory.

A working knowledge of KiCad is assumed.

The version of KiCad currently used for Liverpool Ringing Simulator Project PCB development is 9.0.1.

Libraries
=========

Unpack the libraries zipfile into a convenient location, wherever you keep your custom KiCad libraries, and start KiCad 9.x.

From the project browser window, use the "Preferences | Manage Symbol Libraries..." menu option to register the symbol library file "LRSP_Simulator.kicad_sym" as a Global symbol library with the nickname "LRSP_Simulator".

From the project browser window, use the "Preferences | Manage Footprint Libraries..." menu option to register the footprint library directory "LRSP_Simulator.pretty" as a Global footprint library with the nickname "LRSP_Simulator".

The footprint library uses the KiCad 9.x embedded file functionality to embed 3D models into the library. There is no need for a separate 3D models collection.

PCB Projects
============

Each PCB zipfile is a compressed KiCad project, created with the "File | Archive Project..." menu option from the KiCad project browser.

Download the zipfile for the PCB(s) you want to work on, and create an empty directory to hold each PCB project.

Use the "File | Unarchive Project..." menu option from the KiCad project browser window to unpack each zipfile into the appropriate directory, then use "File | Open Project..." to load the PCB project.

Gerber & Drill File Creation
============================

The Liverpool Ringing Simulator Project creates Gerbers and drill files using the "Fabrication Toolkit" plugin, which creates a compressed zipfile containing files formatted ready for fabrication by JLCPCB. These Gerbers should be suitable for use by other PCB fabrication houses.

Install this plugin into KiCad using the Plugin and Content Manager (PCM) from the KiCad project browser window, and run it from the icon is places in the PCB Editor toolbar. The output zipfile is placed in the sub-directory "production".

Panelisation
============

The Liverpool Ringing Simulator Project panelises small PCBs using the "KiKit" plugin. Ready-made panelised Gerber files are provided in the Gerbers directory.

KiKit has a two phase installation process. Install the plugin frontend into KiCad using the Plugin and Content Manager (PCM) from the KiCad main window, and the Python backend from the KiCad Comand Prompt window - refer to the KiKit documentation for details.

KiKit panelisation parameter files are provided in the PCB project zipfiles. The small sensor boards use the "KiKit_Panel_6.json" parameter file to create a 2 x 3 panel of 6 boards. The Power and 2nd PC boards use the "KiKit_Panel_4.json" parameter file to create a 2 x 2 panel of 4 boards.

Note that KiKit panelisation is intended to be run from the Standalone PCB Editor, not from within a project.

LRSP
20250422

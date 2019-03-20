# MAIN README

The programs in this folder will be upgraded for Python 3 in the future.

About the programs
==================
* Create log files with AMS data logged from the HAN port on Kaifa (MA304H3E) electricity meter.
* Save the files to disk.
* Present the log data graphically with the Python matplotlib module.

There is no "pretty" user interface. To configure where to store the data, and the log frequency, you need to manually edit the source file parse_write.py.

Get started (Windows 10)
------------------------
Install the latest Python 2.7 from here: https://www.python.org/downloads/
Remember to check the box for installing the Path Environment variable for Python 2.7.

If you installed version 2.7.9 or later, you should already have the pip package installer on your PC.
Now check if pip installer needs upgrading by running: python -m pip install --upgrade pip
If necessary pip can be istalled from here: https://www.makeuseof.com/tag/install-pip-for-python/

Check the already installed packages by running: pip list
Eventually you should have these packages:

backports.functools-lru-cache 1.5, beautifulsoup4 4.7.1, cycler 0.10.0, html5lib 1.0.1, kiwisolver 1.0.1, lxml 4.3.0, matplotlib 2.2.3, numpy 1.15.4, pandas 0.23.4, pip 19.0.1, pyparsing 2.3.1, python-dateutil 2.7.5, pytz 2018.9, setuptools 39.0.1, six 1.12.0, soupsieve 1.7, webencodings 0.5.1

If some package is missing in your list, you can install it with this command: pip install <package name>

You should now have all needed packages to log data to file and view them on PC. However, it is probably better to use another computer to save the log files. For this purpose I bought a Raspberry Pi Model 3 B+. I control it from the PC in "Headless" mode (without keyboard/mouse/monitor). To do the job I suggest the applications PuTTY, pscp and VNC. All are free to download from the internet.

Get started (Raspberry Pi with Raspbian Stretch)
------------------------------------------------
First, get the Pi up and running with a monitor.
Enable VNC server with sudo raspi-config -> Interfacing Options -> VNC
Enable SSH with sudo raspi-config -> Interfacing Options -> SSH

Go here: https://geoffboeing.com/2016/03/scientific-python-raspberry-pi/ and perform steps 1 to 6.

Logging AMS data to file:
-------------------------
* Create a directory on the Pi for the Python program and logs.
* Open parse_write.py in the IDLE editor.
* Edit the variable folderName, to contain the path to your chosen folder.
* Start the program from terminal by entering: python parse_write.py
* Datalogging should now start, on a file named the current YYYY-MM-DD.
* When the date changes a new log file is generated, with name equal to the new current date.
* Stop logging with ctrl + f6.

Plot the recorded data:
-----------------------
* On the PC, open main_prog_en.py (or main_prog_no.py) in the IDLE editor.
* Edit the variable folder to contain the path to your chosen folder.
* Start the program with f5. An entry window should appear.
* Check that the path to the log files is the correct one.
* Select up to two dates, and which graphs you want to be shown, then click Ok.
* The graphs should show, and you can now manipulate them with zooming, panning etc.

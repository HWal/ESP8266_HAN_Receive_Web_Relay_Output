# MAIN README

This repository will be upgraded for Python 3 in the future.

About the programs in this repository
=====================================
* Create log files with AMS data logged from the HAN port on Kaifa (MA304H3E) electricity meter.
* Save the files to disk on PC.
* Present the log data graphically with the Python matplotlib module.

There is no "pretty" user interface. To configure where to store the data, and the log frequency, you need to manually edit the source files.

Get started (Windows 10)
------------------------
Install the latest Python 2.7 from here: https://www.python.org/downloads/
Remember to check the box for installing the Path Environment variable for Python 2.7.

If you installed version 2.7.9 or later, you should already have the pip package installer on your PC.
Now check if pip installer needs upgrading by running: python -m pip install --upgrade pip
If necessary pip can be istalled from here: https://www.makeuseof.com/tag/install-pip-for-python/

Check the already installed packages by running: pip list
Eventually you should have these packages:

Package                       Version
----------------------------- -------
backports.functools-lru-cache 1.5
beautifulsoup4                4.7.1
cycler                        0.10.0
html5lib                      1.0.1
kiwisolver                    1.0.1
lxml                          4.3.0
matplotlib                    2.2.3
numpy                         1.15.4
pandas                        0.23.4
pip                           19.0.1
pyparsing                     2.3.1
python-dateutil               2.7.5
pytz                          2018.9
setuptools                    39.0.1
six                           1.12.0
soupsieve                     1.7
webencodings                  0.5.1

If some package is missing in your list, you can install it with this command: pip install <package name>

Logging AMS data to file:
-------------------------
* Create a folder, preferable named "Logs" at a location of your choice.
* Open the file parse_write.py with the IDLE editor, and change line 16 to contain the path to your chosen folder.
* To prevent the program from running forever, edit line 21 to set the maximum number of recordings.
* Start the program with f5. Datalogging should now start, on a file named the current YYYY-MM-DD.
* When the date changes, a new log file should be generated, with name equal to the new current date.
* Stop the logging with ctrl + f6.

Plot the recorded data:
-----------------------
* Open the file main_prog_en.py (or main_prog_no.py) in the IDLE editor.
* Change line 103 to contain the path to your chosen folder.
* Start the program with f5. An entry window should appear.
* Check that the path to the log files is the correct one.
* Select up to two dates, and which graphs you want to be shown, then click Ok.
* The graphs should show, and you can now manipulate them with zooming, panning etc.

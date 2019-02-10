###############################################################
# Plot data collected from AMS smart electricity meter        #
###############################################################

from datetime import datetime
import matplotlib.pyplot as plt
from graphics import *
import plot_power
import plot_voltage
import plot_current
import sys

def input_values(folder, date1, power1, volt1, current1, date2, power2, volt2, current2, win_in):
    # Create entry fields and show current values
    folder_entry = Entry(Point(505, 85), 50)
    folder_entry.setText(folder)
    folder_entry.draw(win_in)
    date_entry1 = Entry(Point(324, 150), 10)
    date_entry1.setText(date1)
    date_entry1.draw(win_in)
    power_entry1 = Entry(Point(720, 150), 2)
    power_entry1.setText(power1)
    power_entry1.draw(win_in)
    volt_entry1 = Entry(Point(720, 180), 2)
    volt_entry1.setText(volt1)
    volt_entry1.draw(win_in)
    current_entry1 = Entry(Point(720, 210), 2)
    current_entry1.setText(current1)
    current_entry1.draw(win_in)
    date_entry2 = Entry(Point(324, 260), 10)
    date_entry2.setText(date2)
    date_entry2.draw(win_in)
    power_entry2 = Entry(Point(720, 260), 2)
    power_entry2.setText(power2)
    power_entry2.draw(win_in)
    volt_entry2 = Entry(Point(720, 290), 2)
    volt_entry2.setText(volt2)
    volt_entry2.draw(win_in)
    current_entry2 = Entry(Point(720, 320), 2)
    current_entry2.setText(current2)
    current_entry2.draw(win_in)
    # Get mouse click and position
    click = win_in.getMouse()
    a = click.getX()
    b = click.getY()
    # Get values from entry fields
    folder = folder_entry.getText()
    date1 = date_entry1.getText()
    power1 = power_entry1.getText()
    volt1 = volt_entry1.getText()
    current1 = current_entry1.getText()
    date2 = date_entry2.getText()
    power2 = power_entry2.getText()
    volt2 = volt_entry2.getText()
    current2 = current_entry2.getText()
    # Check if user wants to quit the app
    if (a > 620 and a < 730) and (b > 389 and b < 410):
        win_in.close()
        plt.close('all')
        sys.exit()
    return folder, date1, power1, volt1, current1, date2, power2, volt2, current2


def main():
    # Create initial window
    win_in = GraphWin("Control window", 800, 450)
    info_text1 = Text(Point(400, 35), "PLOT DATA FROM KAIFA MA304H3E SMART METER")
    info_text1.draw(win_in)
    folder_text = Text(Point(96, 85), "Path to log files:")
    folder_text.draw(win_in)
    date_text1 = Text(Point(137, 150), "Date no. 1 (YYYY-MM-DD):")
    date_text1.draw(win_in)
    power_text1 = Text(Point(583, 150), "Power graph (Y/N):")
    power_text1.draw(win_in)
    volt_text1 = Text(Point(588, 180), "Voltage graph (Y/N):")
    volt_text1.draw(win_in)
    current_text1 = Text(Point(587, 210), "Current graph (Y/N):")
    current_text1.draw(win_in)
    date_text2 = Text(Point(137, 260), "Date no. 2 (YYYY-MM-DD):")
    date_text2.draw(win_in)
    power_text2 = Text(Point(583, 260), "Power graph (Y/N):")
    power_text2.draw(win_in)
    volt_text2 = Text(Point(588, 290), "Voltage graph (Y/N):")
    volt_text2.draw(win_in)
    current_text2 = Text(Point(587, 320), "Current graph (Y/N):")
    current_text2.draw(win_in)
    info_text2 = Text(Point(132, 400), "Enter values and click:")
    info_text2.draw(win_in)
    ok_text = Text(Point(270, 400), "Ok")
    ok_text.draw(win_in)
    p1 = Point(240, 410)
    p2 = Point(300, 389)
    rect1 = Rectangle(p1, p2)
    rect1.draw(win_in)
    finish_text = Text(Point(675, 400), "Exit")
    finish_text.draw(win_in)
    p3 = Point(620, 410)
    p4 = Point(730, 389)
    rect2 = Rectangle(p3, p4)
    rect2.draw(win_in)
    
    # set default values
    folder = "C:\\Users\\HelgeN550JK\\Desktop\\AMS\\python\\logs\\"
    date1 = "2019-01-20"
    date2 = "2019-01-30"
    power1 = volt1 = current1 = power2 = volt2 = current2 = "N"

    # Eternal loop
    while True:
        # Send current values and get new input values
        folder, date1, power1, volt1, current1, date2, power2, volt2, current2 = input_values(folder, date1, power1, volt1, current1, date2, power2, volt2, current2, win_in)
        if power1 == "Y" or power1 == "y":
            plot_power.main(folder + date1 + ".txt")
        if volt1 == "Y" or volt1 == "y":
            plot_voltage.main(folder + date1 + ".txt")
        if current1 == "Y" or current1 == "y":
            plot_current.main(folder + date1 + ".txt")
        if power2 == "Y" or power2 == "y":
            plot_power.main(folder + date2 + ".txt")
        if volt2 == "Y" or volt2 == "y":
            plot_voltage.main(folder + date2 + ".txt")
        if current2 == "Y" or current2 == "y":
            plot_current.main(folder + date2 + ".txt")
        if date1 == "2099-12-31" or date2 == "2099-12-31":
            win_in.close()
            plt.close('all')
            sys.exit()
        # Reset plot choices to default values
        power1 = volt1 = current1 = power2 = volt2 = current2 = "N"

if __name__ == "__main__":
    main()

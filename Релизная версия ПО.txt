import serial
from serial.tools import list_ports
import tkinter
import tkinter.messagebox
import tkintermapview
from tkdial import Meter
import customtkinter
import time
from datetime import datetime
import matplotlib.dates
import os
import pandas as pd
import numpy as np
import math
import re
import threading
from matplotlib.backends.backend_tkagg import (
    FigureCanvasTkAgg, NavigationToolbar2Tk)
from matplotlib.backend_bases import key_press_handler
from matplotlib.figure import Figure
import matplotlib.ticker as ticker
import ctypes
from win32com.shell import shell, shellcon
from tkinter import filedialog
from geopy.distance import geodesic
import pathlib
import os.path

customtkinter.set_appearance_mode("Dark")  # Modes: "System" (standard), "Dark", "Light"
customtkinter.set_default_color_theme("green")  # Themes: "blue" (standard), "green", "dark-blue"
appdir = pathlib.Path(__file__).parent.resolve()
class App(customtkinter.CTk):
    def __init__(self):
        super().__init__()
        self.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.ser = serial.Serial()
        self.ser.baudrate = 115200
        self.desktop_path = shell.SHGetKnownFolderPath(shellcon.FOLDERID_Desktop)
        self.path_papka = os.path.dirname(os.path.realpath('Baikal_boat.exe'))
        self.base_path = os.path.join(self.path_papka, 'Baikal_boat')
        if os.path.exists(self.base_path):
            if os.path.exists(os.path.join(self.base_path, 'data_files')):
                pass
            else:
                os.mkdir(os.path.join(self.base_path, 'data_files'))
        else:
            os.mkdir(self.base_path)
            os.mkdir(os.path.join(self.base_path, 'data_files'))
        if os.path.exists(os.path.join(self.base_path, 'data_base')):
            pass
        else:
            os.mkdir(os.path.join(self.base_path, 'data_base'))
        self.path = self.base_path + '\data_files\output'
        self.log_path = self.base_path + '\data_files\log'
        self.calibration_path = self.base_path + '\data_files\calibration.txt'
        self.df = pd.DataFrame(
            {'Date': [], 'Time': [], 'Date_Time': [], 'Lat': [], 'Lon': [], 'UTime': [],'Aim_Lat': [], 'Aim_Lon': [],
             'Distance': [], 'Azimuth': [], 'X': [], 'Y': [], 'Z': [],'X_calib': [], 'Y_calib': [], 'Z_calib': [],
             'Pitch': [], 'Roll': [],
             'Angle': [], 'Press': [], 'Temp': [], 'Alt': [], 'T1': [], 'T2': [], 'T3': [], 'T4': []})
        self.calibration_df = pd.DataFrame({'X': [], 'Y': [], 'Z': []})
        self.calibration_constants = dict.fromkeys(['x_mul', 'y_mul', 'z_mul', 'x_zero', 'y_zero', 'z_zero'])
        self.x = []
        self.y = []
        self.z = []
        self.temperature = []
        self.pressure = []
        self.time_temp = []
        self.time_press = []
        self.amount_of_points = 25
        self.current_angle = 0
        self.distance = 0
        self.azimuth = 0
        self.navigation_route_pointer = 0
        self.navigation_route_flag = False
        self.receiving_running = False
        self.reconnect_flag = False
        self.current_com = ''
        self.window_width = 1280
        self.window_height = 720
        self.scale_factor = ctypes.windll.shcore.GetScaleFactorForDevice(0) / 125
        self.tk.call('tk', 'scaling', ctypes.windll.shcore.GetScaleFactorForDevice(0) / 100)
        # configure window
        self.title("Baikal boat")
        self.geometry(f'{self.window_width}x{self.window_height}')
        self.resizable(False, False)
        self.iconbitmap(os.path.join(appdir,'logo.ico'))
        # self.configure(fg_color = 'green')
        self.grid_rowconfigure((0, 1), weight=1)
        self.grid_columnconfigure(0, weight=1)

        self.tabview = customtkinter.CTkTabview(self, command = self.tabview_event)
        self.tabview.grid(row=0, column=0, columnspan=1, sticky="nsew")
        self.tabview.add("Настройки")
        self.tabview.add("Данные")
        self.tabview.add("Карта")
        self.tabview.set("Настройки")
        self.tabview._segmented_button.configure(font=customtkinter.CTkFont(size=15, weight="bold"))
        self.tabview.tab("Настройки").grid_columnconfigure((0, 1, 2, 3, 4), weight=1)
        self.tabview.tab("Настройки").grid_rowconfigure((0, 1, 2), weight=1)
        self.tabview.tab("Данные").grid_columnconfigure((0, 1), weight=1)
        self.tabview.tab("Данные").grid_rowconfigure((0, 1), weight=1)
        self.tabview.tab("Карта").grid_columnconfigure((0,1,2), weight=1)
        self.tabview.tab("Карта").grid_rowconfigure((0,1), weight=1)

        self.connection_frame = customtkinter.CTkFrame(self.tabview.tab("Настройки"), fg_color='green',
                                                       corner_radius=10)
        self.connection_frame.grid(row=1, column=0, rowspan=2, padx=(5, 0),
                                   pady=(0, 5), sticky="nsew")
        self.connection_frame.grid_columnconfigure((0, 1), weight=1)
        self.connection_frame.grid_rowconfigure((0, 1, 2, 3, 4), weight=1)

        self.calibration_frame = customtkinter.CTkFrame(self.tabview.tab("Настройки"), width=405,
                                                        height=400, fg_color='green',
                                                        corner_radius=10)
        self.calibration_frame.grid(row=1, column=4, rowspan=2, padx=(0, 5),
                                    pady=(0, 5), sticky="nsew")
        self.calibration_frame.grid_columnconfigure((0, 1, 2), weight=1)
        self.calibration_frame.grid_rowconfigure((0, 1, 2, 3), weight=1)

        self.connection_logo_label = customtkinter.CTkLabel(self.tabview.tab("Настройки"), text="Настройка подключения",
                                                            font=customtkinter.CTkFont(size=20, weight="bold"))
        self.connection_logo_label.grid(row=0, column=0, padx=(0, 0),
                                        pady=(15, 15), sticky="news")

        self.route_logo_label = customtkinter.CTkLabel(self.tabview.tab("Настройки"), text="Сырые данные",
                                                       font=customtkinter.CTkFont(size=20, weight="bold"))
        self.route_logo_label.grid(row=0, column=1, columnspan=3, padx=(0, 0),
                                   pady=(15, 15), sticky="news")

        self.calibration_logo_label = customtkinter.CTkLabel(self.tabview.tab("Настройки"), text="Калибровка",
                                                             font=customtkinter.CTkFont(size=20, weight="bold"))
        self.calibration_logo_label.grid(row=0, column=4, padx=(0, 0),
                                         pady=(15, 15), sticky="news")

        self.com_menue = customtkinter.CTkOptionMenu(self.connection_frame, dynamic_resizing=False,
                                                     command=self.optionmenu_callback)

        self.com_menue.grid(row=0, column=0, columnspan=2, padx=(5, 5),
                            pady=(15, 10), sticky="ew")

        self.connect_button = customtkinter.CTkButton(self.connection_frame, text='Подключение',
                                                      command=self.connect_button_event)
        self.connect_button.grid(row=1, column=0, padx=(5, 5),
                                 pady=(0, 10), sticky="ew")

        self.update_button = customtkinter.CTkButton(self.connection_frame, text='Обновить',
                                                     command=self.update_button_event)
        self.update_button.grid(row=1, column=1, padx=(0, 5), pady=(0, 10),
                                sticky="ew")

        self.route_button = customtkinter.CTkButton(self.connection_frame, text='Выбор маршрута',
                                                    command=self.route_button_event)
        self.route_button.grid(row=2, column=0, columnspan=2, padx=(5, 5), pady=(0, 10),
                               sticky="ew")

        self.log_window = customtkinter.CTkTextbox(self.connection_frame, height=445, width=400)
        self.log_window.grid(row=3, column=0, columnspan=2, padx=(5, 5),
                             pady=(0, 0), sticky="nsew")

        self.clear_log_button = customtkinter.CTkButton(self.connection_frame, text='Очистка',
                                                        command=self.clear_log_button_event)
        self.clear_log_button.grid(row=4, column=0, columnspan=2, padx=(5, 5),
                                   pady=(10, 10), sticky="ew")

        self.raw_data_window = customtkinter.CTkTextbox(self.tabview.tab("Настройки"), height=560,
                                                        width=410, corner_radius=10)
        self.raw_data_window.grid(row=1, column=1, columnspan=3, padx=(10, 10),
                                  pady=(0, 10), sticky="nsew")

        self.start_data_receive_button = customtkinter.CTkButton(self.tabview.tab("Настройки"),
                                                                 width=120, text='Начать прием',
                                                                 command=self.start_data_receive_button_event)

        self.start_data_receive_button.grid(row=2, column=1, padx=(10, 5),
                                            pady=(0, 5), sticky="nsew")
        self.stop_data_receive_button = customtkinter.CTkButton(self.tabview.tab("Настройки"),
                                                                width=120, text='Закончить прием',
                                                                command=self.stop_data_receive_button_event)
        self.stop_data_receive_button.grid(row=2, column=2, padx=(0, 5),
                                           pady=(0, 5), sticky="nswe")
        self.clear_data_button = customtkinter.CTkButton(self.tabview.tab("Настройки"), width=120,
                                                         text='Очистить', command=self.clear_data_button_event)
        self.clear_data_button.grid(row=2, column=3, padx=(0, 10), pady=(0, 5),
                                    sticky="nswe")

        self.calibration_radio_var = tkinter.IntVar(value=1)
        self.calibration_radiobutton_1 = customtkinter.CTkRadioButton(self.calibration_frame, text="Вкл ",
                                                                      variable=self.calibration_radio_var,
                                                                      command=self.calibration_radiobutton_event,
                                                                      value=0)
        self.calibration_radiobutton_2 = customtkinter.CTkRadioButton(self.calibration_frame, text="Выкл",
                                                                      variable=self.calibration_radio_var,
                                                                      command=self.calibration_radiobutton_event,
                                                                      value=1)
        self.calibration_radiobutton_1.grid(row=0, column=0, padx=(115, 0),
                                            pady=(15, 10), sticky="sn")
        self.calibration_radiobutton_2.grid(row=0, column=1, padx=(5, 0),
                                            pady=(15, 10), sticky="sn")

        self.start_calibration_button = customtkinter.CTkButton(self.calibration_frame, text='Начать калибровку',
                                                                width=200,
                                                                command=self.start_calibration_button_event)
        self.start_calibration_button.grid(row=1, column=0, columnspan=1,
                                           padx=(5, 5),
                                           pady=(0, 10), sticky="ew")
        self.stop_calibration_button = customtkinter.CTkButton(self.calibration_frame, text='Очистка',
                                                               width=200,
                                                               command=self.stop_calibration_button_event)
        self.stop_calibration_button.grid(row=1, column=1, columnspan=1, padx=(0, 5),
                                          pady=(0, 10), sticky="ew")
        self.calibration_label = customtkinter.CTkLabel(self.calibration_frame, text='Кол-во точек: 0',
                                                        width=405,
                                                        font=customtkinter.CTkFont(size=17, weight="bold"))
        self.calibration_label.grid(row=4, column=0, columnspan=2, padx=(5, 5),
                                    pady=(0, 5), sticky="snew")

        self.calibration_mult_x_label = customtkinter.CTkLabel(self.calibration_frame,
                                                               text=list(self.calibration_constants.keys())[0],
                                                               font=customtkinter.CTkFont(size=17, weight="bold"))
        self.calibration_mult_x_label.grid(row=5, column=0, columnspan=1,
                                           padx=(5, 5),
                                           pady=(0, 5), sticky="snew")
        self.calibration_mult_y_label = customtkinter.CTkLabel(self.calibration_frame,
                                                               text=list(self.calibration_constants.keys())[1],
                                                               font=customtkinter.CTkFont(size=17, weight="bold"))
        self.calibration_mult_y_label.grid(row=6, column=0, columnspan=1,
                                           padx=(5, 5),
                                           pady=(0, 5), sticky="snew")
        self.calibration_mult_z_label = customtkinter.CTkLabel(self.calibration_frame,
                                                               text=list(self.calibration_constants.keys())[2],
                                                               font=customtkinter.CTkFont(size=17, weight="bold"))
        self.calibration_mult_z_label.grid(row=7, column=0, columnspan=1,
                                           padx=(5, 5),
                                           pady=(0, 5), sticky="snew")
        self.calibration_zero_correction_x_label = customtkinter.CTkLabel(self.calibration_frame,
                                                                          text=list(self.calibration_constants.keys())[
                                                                              3], font=customtkinter.CTkFont(size=17,
                                                                                                             weight="bold"))
        self.calibration_zero_correction_x_label.grid(row=5, column=1, columnspan=1,
                                                      padx=(5, 5),
                                                      pady=(0, 5), sticky="snew")
        self.calibration_zero_correction_y_label = customtkinter.CTkLabel(self.calibration_frame,
                                                                          text=list(self.calibration_constants.keys())[
                                                                              4], font=customtkinter.CTkFont(size=17,
                                                                                                             weight="bold"))
        self.calibration_zero_correction_y_label.grid(row=6, column=1, columnspan=1,
                                                      padx=(5, 5),
                                                      pady=(0, 5), sticky="snew")
        self.calibration_zero_correction_z_label = customtkinter.CTkLabel(self.calibration_frame,
                                                                          text=list(self.calibration_constants.keys())[
                                                                              5], font=customtkinter.CTkFont(size=17,
                                                                                                             weight="bold"))
        self.calibration_zero_correction_z_label.grid(row=7, column=1, columnspan=1,
                                                      padx=(5, 5),
                                                      pady=(0, 5), sticky="snew")

        self.calibration_fig = Figure(figsize=(4, 4), dpi=100)
        self.calibration_ax = self.calibration_fig.add_subplot(111, projection='3d')
        # self.calibration_fig.subplots_adjust(left=0.01, bottom=0.04, top=0.97, hspace=0.13)
        self.calibration_canvas = FigureCanvasTkAgg(self.calibration_fig,
                                                    master=self.calibration_frame)  # A tk.DrawingArea.
        self.calibration_canvas.get_tk_widget().grid(row=2, column=0, columnspan=2,
                                                     padx=(5, 5), pady=(0, 0),
                                                     sticky="snew")
        self.calibration_toolbar = NavigationToolbar2Tk(self.calibration_canvas, self.calibration_frame,
                                                        pack_toolbar=False)
        self.calibration_toolbar.grid(row=3, column=0, columnspan=2,
                                      padx=(5, 5),
                                      pady=(0, 5), sticky="snew")
        self.calibration_toolbar.update()
        self.calibration_ax.set_xlabel('X axis')
        self.calibration_ax.set_ylabel('Y axis')
        self.calibration_ax.set_zlabel('Z axis')

        self.data_logo_label = customtkinter.CTkLabel(self.tabview.tab("Данные"), text="Графики",
                                                      font=customtkinter.CTkFont(size=20, weight="bold"))
        self.data_logo_label.grid(row=0, column=0, rowspan=1, columnspan=1, padx=(0, 0),
                                  pady=(15, 15), sticky="nsew")
        self.data_logo_label = customtkinter.CTkLabel(self.tabview.tab("Данные"), text="Текущие данные",
                                                      font=customtkinter.CTkFont(size=20, weight="bold"))
        self.data_logo_label.grid(row=0, column=1, rowspan=1, columnspan=1, padx=(10, 0),
                                  pady=(15, 15), sticky="nsew")
        self.graph_data_frame = customtkinter.CTkFrame(self.tabview.tab("Данные"), fg_color='green',
                                                       corner_radius=10)
        self.graph_data_frame.grid(row=1, column=0, rowspan=2, padx=(5, 0),
                                   pady=(0, 5), sticky="nsew")
        self.data_frame = customtkinter.CTkFrame(self.tabview.tab("Данные"), fg_color='darkgreen',
                                                 height=1000 * self.scale_factor,
                                                 corner_radius=10)
        self.data_frame.grid(row=1, column=1, rowspan=2, padx=(10, 5),
                             pady=(0, 5), sticky="nsew")
        self.data_frame.grid_rowconfigure((0, 1, 2, 3, 4, 5, 6, 7, 8, 9), weight=1)
        self.data_frame.grid_columnconfigure((0, 1), weight=1)

        self.label_1 = customtkinter.CTkLabel(self.data_frame, text="T1",
                                              font=customtkinter.CTkFont(size=20, weight="bold"))
        self.label_1.grid(row=0, column=0, rowspan=1, columnspan=1, padx=(5, 5),
                          pady=(10, 10), sticky="nsw")
        self.label_2 = customtkinter.CTkLabel(self.data_frame, text="T2",
                                              font=customtkinter.CTkFont(size=20, weight="bold"))
        self.label_2.grid(row=1, column=0, rowspan=1, columnspan=1, padx=(5, 5),
                          pady=(10, 10), sticky="nsw")
        self.label_3 = customtkinter.CTkLabel(self.data_frame, text="T3",
                                              font=customtkinter.CTkFont(size=20, weight="bold"))
        self.label_3.grid(row=2, column=0, rowspan=1, columnspan=1, padx=(5, 5),
                          pady=(10, 10), sticky="nsw")
        self.label_4 = customtkinter.CTkLabel(self.data_frame, text="T4",
                                              font=customtkinter.CTkFont(size=20, weight="bold"))
        self.label_4.grid(row=3, column=0, rowspan=1, columnspan=3, padx=(5, 5),
                          pady=(10, 10), sticky="nsw")
        self.label_5 = customtkinter.CTkLabel(self.data_frame, text="Press",
                                              font=customtkinter.CTkFont(size=20, weight="bold"))
        self.label_5.grid(row=4, column=0, rowspan=1, columnspan=3, padx=(5, 5),
                          pady=(10, 10), sticky="nsw")
        self.label_6 = customtkinter.CTkLabel(self.data_frame, text="Temp",
                                              font=customtkinter.CTkFont(size=20, weight="bold"))
        self.label_6.grid(row=5, column=0, rowspan=1, columnspan=3, padx=(5, 5),
                          pady=(10, 10), sticky="nsw")
        self.empty_label = customtkinter.CTkLabel(self.data_frame, text="", height=300, width=250 * self.scale_factor,
                                                  font=customtkinter.CTkFont(size=20, weight="bold"))
        self.empty_label.grid(row=6, column=0, rowspan=1, columnspan=3,
                              padx=(5, 5),
                              pady=(10, 10), sticky="nsew")

        self.graph_data_frame.grid_rowconfigure((0, 1, 2, 3, 4, 5, 6, 7, 8), weight=1)
        self.graph_data_frame.grid_columnconfigure((0, 1), weight=1)
        self.temperature_fig = Figure(figsize=(13.3, 2.9), dpi=100, facecolor='#917FB3')
        self.temperature_fig.subplots_adjust(left=0.067, right=0.98, bottom=0.245, top=0.94, hspace=0.25)
        self.temperature_ax = self.temperature_fig.add_subplot()
        self.temperature_canvas = FigureCanvasTkAgg(self.temperature_fig,
                                                    master=self.graph_data_frame)  # A tk.DrawingArea.
        self.temperature_canvas.get_tk_widget().grid(row=0, column=0, rowspan=2, columnspan=2,
                                                     padx=(5, 5),
                                                     pady=(15, 0), sticky="snew")
        self.temperature_toolbar = NavigationToolbar2Tk(self.temperature_canvas, self.graph_data_frame,
                                                        pack_toolbar=False)
        self.temperature_toolbar.grid(row=2, column=0, columnspan=2,
                                      padx=(5, 5), pady=(0, 0), sticky="snew")
        self.temperature_toolbar.update()
        self.temperature_ax.grid(visible='True')
        self.temperature_ax.set_xlabel('Время')
        self.temperature_ax.set_ylabel('Температура, °C')
        self.temperature_radio_var = tkinter.IntVar(value=0)
        self.temperature_radiobutton_1 = customtkinter.CTkRadioButton(self.graph_data_frame, text="Live", height=1,
                                                                      variable=self.temperature_radio_var,
                                                                      command=self.temperature_radiobutton_event,
                                                                      value=0)
        self.temperature_radiobutton_2 = customtkinter.CTkRadioButton(self.graph_data_frame, text="Пауза",
                                                                      variable=self.temperature_radio_var,
                                                                      command=self.temperature_radiobutton_event,
                                                                      value=1)
        self.temperature_radiobutton_1.grid(row=3, column=0, padx=(350, 5),
                                            pady=(5, 5), sticky="snew")
        self.temperature_radiobutton_2.grid(row=3, column=1, padx=(5, 5),
                                            pady=(5, 5), sticky="snew")

        self.pressure_fig = Figure(figsize=(13.3, 2.9), dpi=100, facecolor='#917FB3')
        self.pressure_fig.subplots_adjust(left=0.067, right=0.98, bottom=0.245, top=0.94, hspace=0.25)
        self.pressure_ax = self.pressure_fig.add_subplot()
        self.pressure_canvas = FigureCanvasTkAgg(self.pressure_fig, master=self.graph_data_frame)  # A tk.DrawingArea.
        self.pressure_canvas.get_tk_widget().grid(row=5, column=0, rowspan=2, columnspan=2,
                                                  padx=(5, 5), pady=(0, 0),
                                                  sticky="snew")
        self.pressure_toolbar = NavigationToolbar2Tk(self.pressure_canvas, self.graph_data_frame, pack_toolbar=False)
        self.pressure_toolbar.grid(row=7, column=0, columnspan=2, padx=(5, 5),
                                   pady=(0, 0), sticky="snew")
        self.pressure_toolbar.update()
        self.pressure_ax.grid(visible='True')
        self.pressure_ax.set_xlabel('Время')
        self.pressure_ax.set_ylabel('Давление, Па')
        self.pressure_radio_var = tkinter.IntVar(value=0)
        self.pressure_radiobutton_1 = customtkinter.CTkRadioButton(self.graph_data_frame, text="Live",
                                                                   variable=self.pressure_radio_var,
                                                                   command=self.pressure_radiobutton_event, value=0)
        self.pressure_radiobutton_2 = customtkinter.CTkRadioButton(self.graph_data_frame, text="Пауза",
                                                                   variable=self.pressure_radio_var,
                                                                   command=self.pressure_radiobutton_event, value=1)
        self.pressure_radiobutton_1.grid(row=8, column=0, padx=(350, 5),
                                         pady=(5, 5), sticky="snew")
        self.pressure_radiobutton_2.grid(row=8, column=1, padx=(5, 5),
                                         pady=(5, 5), sticky="snew")

        self.database_path = self.base_path + '\data_base\offline_map.db'
        if os.path.exists(self.database_path):
            self.map_widget = tkintermapview.TkinterMapView(self.tabview.tab("Карта"), height=800 * self.scale_factor,
                                                            corner_radius=10, use_database_only=True,
                                                            database_path=self.database_path, max_zoom=18)
        else:
            self.map_widget = tkintermapview.TkinterMapView(self.tabview.tab("Карта"), height=800 * self.scale_factor,
                                                            corner_radius=10)
        self.map_widget.grid(row=0, column=0, padx=(5, 7), pady=(10, 10), columnspan=3, sticky='news')
        # self.map_widget.set_tile_server("https://mt0.google.com/vt/lyrs=s&hl=en&x={x}&y={y}&z={z}&s=Ga", max_zoom=22)  # google satellite
        # self.map_widget.set_tile_server("https://mt0.google.com/vt/lyrs=m&hl=en&x={x}&y={y}&z={z}&s=Ga", max_zoom=22)  # google normal
        self.map_widget.set_position(56.5158043, 85.0505219)  # Tomsk
        self.map_widget.set_zoom(15)
        self.compass = Meter(self.tabview.tab("Карта"), fg="#242424", radius=250 * self.scale_factor, start=0, end=360,
                             major_divisions=30, minor_divisions=10, border_width=5, text_color="white",
                             start_angle=90, end_angle=-360, scale_color="white", axis_color="gray",
                             needle_color="red", text_font=customtkinter.CTkFont(size=20))
        self.compass.configure(state = 'Unbind')
        self.compass.grid(row=0, column=2, padx=(0, 25), pady=(0, 25), sticky='es')
        self.current_distance_label = customtkinter.CTkLabel(self.tabview.tab("Карта"), text="Текущее расстояние до цели: None",
                                                  font=customtkinter.CTkFont(size=20, weight="bold"))
        self.current_distance_label.grid(row=1, column=0, sticky='news')
        self.current_azimuth_label = customtkinter.CTkLabel(self.tabview.tab("Карта"), text="Целевой курс: None",
                                                  font=customtkinter.CTkFont(size=20, weight="bold"))
        self.current_azimuth_label.grid(row=1, column=1, sticky='news')
        self.current_direct_label = customtkinter.CTkLabel(self.tabview.tab("Карта"), text="Движение: None",
                                                  font=customtkinter.CTkFont(size=20, weight="bold"))
        self.current_direct_label.grid(row=1, column=2, sticky='news')


        # Начальная настройка
        self.update_com()  # устанавливает COM порты при запуске
        self.log_window.configure(state="disabled")
        self.raw_data_window.configure(state="disabled")
        self.parsing_file(self.calibration_path)


        # Конец настройки

    def data_receiving(self):
        try:
            while self.receiving_running:
                data = self.ser.readline().decode()
                prepared_data = self.parsing(data)
                self.data_show(prepared_data, 'T1', self.label_1)
                self.data_show(prepared_data, 'T2', self.label_2)
                self.data_show(prepared_data, 'T3', self.label_3)
                self.data_show(prepared_data, 'T4', self.label_4)
                self.data_show(prepared_data, 'Press', self.label_5)
                self.data_show(prepared_data, 'Temp', self.label_6)
                self.set_data_textbox(self.raw_data_window, str(data))

                if 'Press' in prepared_data.keys():
                    t3 = threading.Thread(target=self.graph_drawing(self.pressure_ax, prepared_data['Press'], prepared_data['Date_Time']))
                    t3.start()

                if 'T1' in prepared_data.keys():
                    t2 = threading.Thread(target=self.graph_drawing(self.temperature_ax,
                                       [prepared_data['T1'], prepared_data['T2'], prepared_data['T3'],
                                        prepared_data['T4']], prepared_data['Date_Time']))
                    t2.start()

                if 'X' and 'Y' and 'Z' in prepared_data.keys():
                    if self.calibration_radio_var.get() == 0:
                        t1 = threading.Thread(target=self.calibration_graph(prepared_data['X'], prepared_data['Y'], prepared_data['Z']))

                        t1.start()


                if 'Lat' and 'Lon' in prepared_data.keys():
                    self.current_route_list = self.df[['Lat', 'Lon']].dropna().to_numpy()
                    if len(self.current_route_list) == 1:
                        pass
                        self.current_route_start_point = self.map_widget.set_marker(self.current_route_list[0][0],
                                                                                    self.current_route_list[0][1],
                                                                                    text="Начальная точка")

                    elif len(self.current_route_list) == 2:
                        self.current_route_path = self.map_widget.set_path(self.current_route_list, width=5,
                                                                           color='blue')
                        self.current_route_current_point = self.map_widget.set_marker(self.current_route_list[-1][0],
                                                                                      self.current_route_list[-1][1])
                        self.current_route_current_point.set_text('Я')

                    elif len(self.current_route_list) > 2:
                        if self.tabview.get() == 'Карта':
                            self.current_route_path.set_position_list(self.current_route_list)
                            self.current_route_current_point.set_position(self.current_route_list[-1][0],
                                                                          self.current_route_list[-1][1])





        except BaseException:
            if self.reconnect_flag == True:
                self.set_data_textbox(self.log_window, 'Ошибка запуска приема данных\n')
                time.sleep(3)
                self.set_data_textbox(self.log_window, 'Попытка переподключиться\n')
                if (self.reconnect() == 1):
                    self.set_data_textbox(self.log_window, 'Успешное переподключение\n')
                self.start_data_receiving()
            else:
                self.receiving_running = False
                self.set_data_textbox(self.log_window, 'Завершение попыток переподключиться\n')

    def start_data_receive_button_event(self):
        self.reconnect_flag = True
        self.ser.close()
        try:
            self.ser.open()
            self.set_data_textbox(self.log_window, 'Попытка запуска приема данных\n')
            if self.receiving_running == False:
                self.start_data_receiving()
            self.set_data_textbox(self.log_window, 'Успешный запуск приема данных\n')
        except BaseException as e:
            self.set_data_textbox(self.log_window, 'Ошибка: COM-порта не существует\n')
            self.receiving_running = False

    def start_data_receiving(self):
        self.receiving_running = True
        t = threading.Thread(target=self.data_receiving)
        t.start()



    def stop_data_receive_button_event(self):
        self.stop_data_receiving()

    def stop_data_receiving(self):
        self.reconnect_flag = False
        self.receiving_running = False
        self.ser.close()
        self.set_data_textbox(self.log_window, 'Прием данных остановлен\n')
        self.df.to_excel(self.path + ' ' + str(time.strftime('%d.%m.%y %H.%M.%S', time.localtime())) + '.xlsx')
        self.df.to_csv(self.path + ' ' + str(time.strftime('%d.%m.%y %H.%M.%S', time.localtime())) + '.csv')
        # self.df.to_excel(self.path + '.xlsx')
        # self.df.to_csv(self.path + '.csv')
        self.set_data_textbox(self.log_window,
                              f"Данные сохранены по пути:\n{self.path + ' ' + str(time.strftime('%d.%m.%y %H.%M.%S', time.localtime())) + '.csv'} \n")


    def start_calibration_button_event(self):
        pass
        self.calibration_constants['x_mul'] = round(
            1000 / (self.calibration_df['X'].max() - self.calibration_df['X'].min()), 3)
        self.calibration_constants['y_mul'] = round(
            1000 / (self.calibration_df['Y'].max() - self.calibration_df['Y'].min()), 3)
        self.calibration_constants['z_mul'] = round(
            1000 / (self.calibration_df['Z'].max() - self.calibration_df['Z'].min()), 3)
        self.calibration_constants['x_zero'] = self.calibration_df['X'].min() + (
                self.calibration_df['X'].max() - self.calibration_df['X'].min()) / 2
        self.calibration_constants['y_zero'] = self.calibration_df['Y'].min() + (
                self.calibration_df['Y'].max() - self.calibration_df['Y'].min()) / 2
        self.calibration_constants['z_zero'] = self.calibration_df['Z'].min() + (
                self.calibration_df['Z'].max() - self.calibration_df['Z'].min()) / 2
        string = ''
        for key, value in self.calibration_constants.items():
            string += (f"{key} = {value} \n")
        self.set_data_textbox(self.log_window, f'Новые калибровочные константы:\n{string}')
        self.calibration_mult_x_label.configure(
            text=f'{list(self.calibration_constants.keys())[0]} = {self.calibration_constants[list(self.calibration_constants.keys())[0]]}')
        self.calibration_mult_y_label.configure(
            text=f'{list(self.calibration_constants.keys())[1]} = {self.calibration_constants[list(self.calibration_constants.keys())[1]]}')
        self.calibration_mult_z_label.configure(
            text=f'{list(self.calibration_constants.keys())[2]} = {self.calibration_constants[list(self.calibration_constants.keys())[2]]}')
        self.calibration_zero_correction_x_label.configure(
            text=f'{list(self.calibration_constants.keys())[3]} = {self.calibration_constants[list(self.calibration_constants.keys())[3]]}')
        self.calibration_zero_correction_y_label.configure(
            text=f'{list(self.calibration_constants.keys())[4]} = {self.calibration_constants[list(self.calibration_constants.keys())[4]]}')
        self.calibration_zero_correction_z_label.configure(
            text=f'{list(self.calibration_constants.keys())[5]} = {self.calibration_constants[list(self.calibration_constants.keys())[5]]}')
        self.set_data_textbox(self.log_window, f'Калибровочные константы сохранены по пути: {self.calibration_path}\n')
        with open(self.calibration_path, "w") as f:
            f.write(string)
        self.calibration_radiobutton_2.invoke()
        if not any(value is None or (isinstance(value, float) and pd.isnull(value)) for value in
                   self.calibration_constants.values()):
            self.calibration_ax.clear()
            self.calibration_ax.scatter((np.array(self.calibration_df['X']) - self.calibration_constants['x_zero']) *
                                        self.calibration_constants['x_mul'],
                                        (np.array(self.calibration_df['Y']) - self.calibration_constants['y_zero']) *
                                        self.calibration_constants['y_mul'],
                                        (np.array(self.calibration_df['Z']) - self.calibration_constants['z_zero']) *
                                        self.calibration_constants['z_mul'], color='#ff7f0e')
            self.calibration_ax.scatter(np.array(self.calibration_df['X']), np.array(self.calibration_df['Y']),
                                        np.array(self.calibration_df['Z']), color='#1f77b4')
            self.calibration_ax.set_xlabel('X axis')
            self.calibration_ax.set_ylabel('Y axis')
            self.calibration_ax.set_zlabel('Z axis')
            self.calibration_ax.grid(visible='True')
            self.calibration_ax.legend(['Откалиброванные', 'Некалиброванные'], loc='upper right',
                                       bbox_to_anchor=(1.3, 1.15))
            self.calibration_canvas.draw()
            self.calibration_label.configure(text=f'Кол-во точек: {len(self.calibration_df)}')

    def stop_calibration_button_event(self):
        pass
        self.set_data_textbox(self.log_window, 'Очистка калибровочных точек\n')
        self.calibration_df.drop(self.calibration_df.index, inplace=True)
        self.calibration_ax.clear()
        self.calibration_ax.set_xlabel('X axis')
        self.calibration_ax.set_ylabel('Y axis')
        self.calibration_ax.set_zlabel('Z axis')
        self.calibration_ax.grid(visible='True')
        self.calibration_canvas.draw()
        self.calibration_label.configure(text=f'Кол-во точек: {len(self.calibration_df)}')

    def calibration_radiobutton_event(self):
        if self.calibration_radio_var.get() == 0:
            self.set_data_textbox(self.log_window, 'Включен сбор точек для калибровки\n')
        elif self.calibration_radio_var.get() == 1:
            self.set_data_textbox(self.log_window, 'Выключен сбор точек для калибровки\n')

    def optionmenu_callback(self, choice):
        self.set_data_textbox(self.log_window, 'Выбран ' + str(choice) + '\n')

    def connect_button_event(self):
        self.connect_to_com()

    def route_button_event(self):
        self.route_path = filedialog.askopenfilename(initialdir=self.base_path, title="Select file",
                                                     filetypes=(("Excel files", "*.xlsx"), ("All files", "*.*")))
        self.navigation_route_pointer = 0
        self.distance = 0
        self.azimuth = 0
        self.navigation_route_flag = False
        self.current_distance_label.configure(text=f'Текущее расстояние до цели: None')
        self.current_azimuth_label.configure(text=f'Целевой курс: None')
        self.current_direct_label.configure(text='Движение: None')
        if self.route_path == '':
            self.set_data_textbox(self.log_window, f'Ошибка! Повторите выбор файла маршрута\n')
            try:
                self.marker_start.delete()
                self.marker_finish.delete()
                self.target_path.delete()
            except BaseException:
                pass
        else:
            self.set_data_textbox(self.log_window, f'Выбранный путь файла маршрута: {self.route_path}\n')
            try:
                self.route_df = pd.read_excel(self.route_path)[['Lat', 'Lon']].dropna()
                self.route_list = self.route_df.to_numpy()
                try:
                    self.marker_start.delete()
                    self.marker_finish.delete()
                    self.target_path.delete()
                except BaseException:
                    pass
                self.map_widget.set_position(self.route_list[0][0], self.route_list[0][1])
                self.map_widget.set_zoom(13)
                self.navigation_route_flag = True
                self.marker_start = self.map_widget.set_marker(self.route_list[0][0], self.route_list[0][1],
                                                               text="Старт")
                self.marker_finish = self.map_widget.set_marker(self.route_list[-1][0], self.route_list[-1][1],
                                                                text="Финиш")
                self.target_path = self.map_widget.set_path(self.route_list, width=5, color='red')
                self.set_data_textbox(self.log_window, f'\n{self.route_df}\n')
            except BaseException:
                self.set_data_textbox(self.log_window, f'Ошибка! Нечитаемые данные маршрута\n')
                self.navigation_route_flag = False
                try:
                    self.marker_start.delete()
                    self.marker_finish.delete()
                    self.target_path.delete()
                except BaseException:
                    pass

    def update_com(self):
        avalible_ports = []
        avalible_ports = self.get_ports()
        self.set_data_menue(self.get_ports())
        if len(avalible_ports) > 0:
            self.com_menue.set('COM порты доступны')
        else:
            self.com_menue.set('COM порты недоступны')

    def update_button_event(self):
        self.set_data_textbox(self.log_window, 'Обновление COM-портов\n')
        self.update_com()
        self.reconnect_flag = False
        self.receiving_running = False
        self.ser.close()
        self.set_data_textbox(self.log_window, 'Прием данных остановлен\n')


    def clear_data_button_event(self):
        self.raw_data_window.configure(state="normal")
        self.raw_data_window.delete("0.0", "end")
        self.raw_data_window.configure(state="disabled")

    def clear_log_button_event(self):

        self.log_window.configure(state="normal")
        self.log_window.delete("0.0", "end")
        self.log_window.configure(state="disabled")

    def set_data_textbox(self, obj, data):

        obj.configure(state="normal")
        obj.insert(index='end', text=time.strftime('%d.%m.%y %H:%M:%S', time.localtime()) + ' - ' + str(data))
        obj.configure(state="disabled")
        if obj._y_scrollbar.get()[1] > 0.9:
            obj.see("end")
        else:
            pass

    def set_data_menue(self, data):

        self.com_menue.configure(values=data)

    def get_ports(self):

        return list(ports.device for ports in list_ports.comports())

    def reconnect(self):
        try:
            self.ser.port = self.current_com
            if not (self.ser.isOpen()):
                self.ser.open()
            if self.ser.isOpen():
                self.set_data_textbox(self.log_window, 'Успешно подключен ' + self.current_com + '\n')
                return 1
        except serial.SerialException as e:
            e_numb = str(e)[-2]
            if e_numb == '2':
                self.set_data_textbox(self.log_window, 'COM порт "' + self.current_com + '" не найден' '\n')
                return 0
            elif e_numb == '5':
                self.set_data_textbox(self.log_window, 'Отказано в доступе "' + self.current_com + '" \n')
                return 0
            else:
                return 0

    def connect_to_com(self):
        try:
            self.current_com = self.com_menue.get()
            self.ser.port = self.current_com
            if not (self.ser.isOpen()):
                self.ser.open()
            if self.ser.isOpen():
                self.set_data_textbox(self.log_window, 'Успешно подключен ' + self.current_com + '\n')
                return 1
        except serial.SerialException as e:
            e_numb = str(e)[-2]
            if e_numb == '2':
                self.set_data_textbox(self.log_window, 'COM порт "' + self.current_com + '" не найден' '\n')
                return 0
            elif e_numb == '5':
                self.set_data_textbox(self.log_window, 'Отказано в доступе "' + self.current_com + '" \n')
                return 0
            else:
                return 0

    def on_closing(self, event=0):

        with open(self.log_path + ' ' + str(time.strftime('%d.%m.%y %H.%M.%S', time.localtime())) + '.txt', "w") as f:
            f.write(self.log_window.get("0.0", "end"))
        self.destroy()
        exit()

    def parsing(self, input_string):
        pattern = r'(\w+)\s=\s([-+]?\d*\.?\d+)'
        matches = re.findall(pattern, input_string)
        # Создаем словарь для хранения пар ключ-значение
        parsing_data = {}
        for match in matches:
            key = match[0]
            value = float(match[1]) if '.' in match[1] else int(match[1])  # Преобразуем значение в число
            parsing_data[key] = value
        parsing_data['Date'] = time.strftime('%d.%m.%y', time.localtime())
        parsing_data['Time'] = time.strftime('%H:%M:%S', time.localtime())
        parsing_data['Date_Time'] = datetime.strptime(time.strftime('%d.%m.%y %H:%M:%S', time.localtime()),
                                                      '%d.%m.%y %H:%M:%S')
        if 'UTime' and 'Lat' and 'Lon' in parsing_data.keys():
            temporary = self.df[['UTime']].dropna().to_numpy()
            try:
                temporary = temporary[-1]
                if temporary == parsing_data['UTime']:
                    pass
                else:
                    parsing_data['Lat'] = parsing_data['Lat'] / 100000
                    parsing_data['Lon'] = parsing_data['Lon'] / 100000
                    if self.navigation_route_flag == True:
                        parsing_data['Aim_Lat'] = self.route_list[self.navigation_route_pointer][0]
                        parsing_data['Aim_Lon'] = self.route_list[self.navigation_route_pointer][1]
                        parsing_data['Distance'], parsing_data['Azimuth'] = self.navigation(parsing_data['Lat'], parsing_data['Lon'])
                        # parsing_data['Distance'], parsing_data['Azimuth'] = self.navigation()

                    self.df.loc[len(self.df)] = parsing_data
            except BaseException:
                parsing_data['Lat'] = parsing_data['Lat'] / 100000
                parsing_data['Lon'] = parsing_data['Lon'] / 100000
                self.df.loc[len(self.df)] = parsing_data
        elif 'X' and 'Y' and 'Z' in parsing_data.keys():
            if not (parsing_data['X'] == 10000 or parsing_data['Y'] == 10000 or parsing_data['Z'] == 10000):
                parsing_data['Pitch'] = parsing_data['Pitch']/100
                parsing_data['Roll'] = parsing_data['Roll'] / 100
                if not any(value is None or (isinstance(value, float) and pd.isnull(value)) for value in
                           self.calibration_constants.values()):
                    parsing_data['X_calib'] = (parsing_data['X'] - self.calibration_constants['x_zero']) * \
                                              self.calibration_constants['x_mul']
                    parsing_data['Y_calib'] = (parsing_data['Y'] - self.calibration_constants['y_zero']) * \
                                              self.calibration_constants['y_mul']
                    parsing_data['Z_calib'] = (parsing_data['Z'] - self.calibration_constants['z_zero']) * \
                                              self.calibration_constants['z_mul']
                    parsing_data['Angle'] = self.convert_to_angle(parsing_data['X_calib'], parsing_data['Y_calib'],
                                                                  parsing_data['Z_calib'],parsing_data['Pitch'], parsing_data['Roll'])
                    self.current_angle = parsing_data['Angle']
                    self.angle_solver(self.azimuth, self.current_angle, 15)
                self.df.loc[len(self.df)] = parsing_data
            else:
                parsing_data = {}
                pass
        else:
            self.df.loc[len(self.df)] = parsing_data

        return parsing_data

    def parsing_file(self, file_path):

        if file_path == self.calibration_path:
            try:
                with open(file_path, "r", encoding="utf-8") as f:
                    file_data = f.read()
                    pattern = r'(\w+)\s=\s([-+]?\d*\.?\d+)'
                    matches = re.findall(pattern, file_data)
                    parsing_data = {}
                    for match in matches:
                        key = match[0]
                        value = float(match[1]) if '.' in match[1] else int(match[1])  # Преобразуем значение в число
                        parsing_data[key] = value
                    for i in parsing_data.keys():
                        if i in self.calibration_constants.keys():
                            self.calibration_constants[i] = parsing_data[i]
                string = ''
                for key, value in self.calibration_constants.items():
                    string += (f"{key} = {value} \n")
                self.set_data_textbox(self.log_window, f'Полученны калибровочные константы:\n{string}')

            except BaseException:
                self.set_data_textbox(self.log_window, 'Нет данных о калибровке\n')

            self.calibration_mult_x_label.configure(
                text=f'{list(self.calibration_constants.keys())[0]} = {self.calibration_constants[list(self.calibration_constants.keys())[0]]}')
            self.calibration_mult_y_label.configure(
                text=f'{list(self.calibration_constants.keys())[1]} = {self.calibration_constants[list(self.calibration_constants.keys())[1]]}')
            self.calibration_mult_z_label.configure(
                text=f'{list(self.calibration_constants.keys())[2]} = {self.calibration_constants[list(self.calibration_constants.keys())[2]]}')
            self.calibration_zero_correction_x_label.configure(
                text=f'{list(self.calibration_constants.keys())[3]} = {self.calibration_constants[list(self.calibration_constants.keys())[3]]}')
            self.calibration_zero_correction_y_label.configure(
                text=f'{list(self.calibration_constants.keys())[4]} = {self.calibration_constants[list(self.calibration_constants.keys())[4]]}')
            self.calibration_zero_correction_z_label.configure(
                text=f'{list(self.calibration_constants.keys())[5]} = {self.calibration_constants[list(self.calibration_constants.keys())[5]]}')

    def convert_to_angle(self, x, y, z, pitch, roll):
        pitch = math.radians(pitch)
        roll = math.radians(roll)
        x_comp = x * math.cos(pitch) + y * math.sin(pitch) * math.sin(roll) + z * math.sin(pitch) * math.cos(roll)
        y_comp = y * math.cos(roll) - z * math.sin(roll)
        angle = math.atan2(x_comp, -y_comp) * 180 / math.pi
        angle = round((angle + 270) % 360, 0)
        self.compass.set(angle)
        return angle

    def navigation(self, lat, lon):
        distance = int(geodesic((lat, lon), (self.route_list[self.navigation_route_pointer][0], self.route_list[self.navigation_route_pointer][1])).meters)
        self.distance = distance
        azimuth = self.azimuth_calc(lat, lon, self.route_list[self.navigation_route_pointer][0],self.route_list[self.navigation_route_pointer][1])
        self.azimuth = azimuth
        self.current_distance_label.configure(text=f'Текущее расстояние до цели: {distance}')
        self.current_azimuth_label.configure(text=f'Целевой курс: {azimuth}')
        # self.angle_solver(azimuth, self.current_angle, 15)
        if distance<5:
            #
            self.navigation_route_pointer += 1
            if self.navigation_route_pointer == len(self.route_list):
                self.current_distance_label.configure(text=f'Текущее расстояние до цели: Маршрут завершен')
                self.current_azimuth_label.configure(text=f'Целевой курс: Маршрут завершен')
                self.navigation_route_flag = False
        else:
            pass
        return distance, azimuth

    def azimuth_calc(self, lat1, lon1, lat2, lon2):
        lat1 = math.radians(lat1)
        lat2 = math.radians(lat2)
        lon1 = math.radians(lon1)
        lon2 = math.radians(lon2)
        dlat = lat2 - lat1
        dlon = lon2 - lon1
        azimuth_ang = math.degrees(math.atan2((math.sin(dlon) * math.cos(lat2)), (math.cos(lat1)*math.sin(lat2) - math.sin(lat1)*math.cos(lat2)*math.cos(dlon))))
        if azimuth_ang<0:
            azimuth_ang+=360
        return round(azimuth_ang,1)

    def angle_solver(self, goal_angle, current_angle, tolerance):
        if self.distance >= 5:
            if ((goal_angle - current_angle + 360) % 360) < tolerance or ((goal_angle - current_angle + 360) % 360) > (360 - tolerance):
                self.current_direct_label.configure(text=f'Движение: ВПЕРЕД')
            elif ((goal_angle - current_angle + 360) % 360) < ((current_angle - goal_angle + 360) % 360):
                self.current_direct_label.configure(text=f'Движение: НАПРАВО')
            elif ((goal_angle - current_angle + 360) % 360) >= ((current_angle - goal_angle + 360) % 360):
                self.current_direct_label.configure(text=f'Движение: НАЛЕВО')
        else:
            self.current_direct_label.configure(text=f'Движение: СТОП')
    def data_show(self, data, key, frame):
        if key in data.keys():
            frame.configure(text=f'{key} = {data[key]}')

    def tabview_event(self):
        if self.tabview.get() == 'Данные':
            try:
                self.update_graph(self.temperature_ax)
                self.update_graph(self.pressure_ax)
            except BaseException:
                pass
        elif self.tabview.get() == 'Карта':
            try:
                self.current_route_path.set_position_list(self.current_route_list)
                self.current_route_current_point.set_position(self.current_route_list[-1][0],
                                                              self.current_route_list[-1][1])
            except BaseException:
                pass
    ### Отрисовка графиков
    def graph_drawing(self, axis, data, time):

        if axis == self.temperature_ax:
            self.time_temp.append(time)
            self.temperature.append(data)
            if self.tabview.get() == 'Данные':
                self.update_graph(axis)
        elif axis == self.pressure_ax:
            self.time_press.append(time)
            self.pressure.append(data)
            if self.tabview.get() == 'Данные':
                self.update_graph(axis)

    def update_graph(self, axis, other=0):
        if axis == self.temperature_ax:
            if self.temperature_radio_var.get() == 0:
                self.temperature_ax.clear()
                self.temperature_ax.plot_date(self.time_temp[-self.amount_of_points:],
                                              self.temperature[-self.amount_of_points:], linestyle='solid', linewidth=3)
                self.temperature_ax.grid(visible='True')
                self.temperature_ax.set_xlabel('Время')
                self.temperature_ax.set_ylabel('Температура, °C')
                self.temperature_ax.legend(['T1', 'T2', 'T3', 'T4'], loc='upper right', framealpha=1)
                self.temperature_ax.xaxis.set_major_locator(matplotlib.dates.AutoDateLocator())
                self.temperature_ax.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%H:%M:%S'))
                self.temperature_ax.yaxis.set_major_locator(ticker.MaxNLocator(6))
                self.temperature_fig.tight_layout()
                self.temperature_canvas.draw()
            elif self.temperature_radio_var.get() == 1:
                pass
        elif axis == self.pressure_ax:
            if self.pressure_radio_var.get() == 0:
                self.pressure_ax.clear()
                self.pressure_ax.plot_date(self.time_press[-self.amount_of_points:],
                                           self.pressure[-self.amount_of_points:], linestyle='solid', linewidth=3)
                self.pressure_ax.grid(visible='True')
                self.pressure_ax.set_xlabel('Время')
                self.pressure_ax.set_ylabel('Давление, Па')
                self.pressure_ax.legend(['Pressure'], loc='upper right', framealpha=1)
                self.pressure_ax.xaxis.set_major_locator(matplotlib.dates.AutoDateLocator())
                self.pressure_ax.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%H:%M:%S'))
                self.pressure_ax.yaxis.get_major_formatter().set_useOffset(False)
                self.pressure_fig.tight_layout()
                self.pressure_canvas.draw()
            elif self.pressure_radio_var.get() == 1:
                pass

    def temperature_radiobutton_event(self):
        if self.temperature_radio_var.get() == 0:
            self.update_graph(self.temperature_ax)
        elif self.temperature_radio_var.get() == 1:
            self.temperature_ax.clear()
            self.temperature_ax.plot_date(self.time_temp[:], self.temperature[:], linestyle='solid', linewidth=3)
            self.temperature_ax.grid(visible='True')
            self.temperature_ax.set_xlabel('Время')
            self.temperature_ax.set_ylabel('Температура, °C')
            self.temperature_ax.xaxis.set_major_locator(matplotlib.dates.AutoDateLocator())
            self.temperature_ax.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%H:%M:%S'))
            self.temperature_ax.yaxis.set_major_locator(ticker.MaxNLocator(6))
            # self.temperature_fig.autofmt_xdate()
            self.temperature_ax.legend(['T1', 'T2', 'T3', 'T4'], loc='upper right', framealpha=1)
            self.temperature_fig.tight_layout()
            self.temperature_canvas.draw()

    def pressure_radiobutton_event(self):
        if self.pressure_radio_var.get() == 0:
            self.update_graph(self.pressure_ax)
        elif self.pressure_radio_var.get() == 1:
            self.pressure_ax.clear()
            self.pressure_ax.plot_date(self.time_press[:], self.pressure[:], linestyle='solid', linewidth=3)
            self.pressure_ax.grid(visible='True')
            self.pressure_ax.set_xlabel('Время')
            self.pressure_ax.set_ylabel('Давление, Па')
            self.pressure_ax.xaxis.set_major_locator(matplotlib.dates.AutoDateLocator())
            self.pressure_ax.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%H:%M:%S'))
            self.pressure_ax.yaxis.set_major_locator(ticker.MaxNLocator(6))
            # self.pressure_fig.autofmt_xdate()
            self.pressure_ax.legend(['Pressure'], loc='upper right', framealpha=1)
            self.pressure_ax.yaxis.get_major_formatter().set_useOffset(False)
            self.pressure_fig.tight_layout()
            self.pressure_canvas.draw()

            ### Конец отрисовки графиков

    ### Начало калибровки
    def calibration_graph(self, x, y, z):
        # self.calibration_fig
        self.calibration_df.loc[len(self.calibration_df)] = {'X': x, 'Y': y, 'Z': z}
        self.calibration_ax.clear()

        if not any(value is None or (isinstance(value, float) and pd.isnull(value)) for value in
                   self.calibration_constants.values()):
            self.calibration_ax.scatter((np.array(self.calibration_df['X']) - self.calibration_constants['x_zero']) *
                                        self.calibration_constants['x_mul'],
                                        (np.array(self.calibration_df['Y']) - self.calibration_constants['y_zero']) *
                                        self.calibration_constants['y_mul'],
                                        (np.array(self.calibration_df['Z']) - self.calibration_constants['z_zero']) *
                                        self.calibration_constants['z_mul'], color='#ff7f0e')
            self.calibration_ax.legend(['Откалиброванные'], loc='upper right', bbox_to_anchor=(1.3, 1.15))
        else:
            self.calibration_ax.scatter(np.array(self.calibration_df['X']), np.array(self.calibration_df['Y']),
                                        np.array(self.calibration_df['Z']), color='#1f77b4')
            self.calibration_ax.legend(['Некалиброванные'], loc='upper right', bbox_to_anchor=(1.3, 1.15))
        self.calibration_ax.set_xlabel('X axis')
        self.calibration_ax.set_ylabel('Y axis')
        self.calibration_ax.set_zlabel('Z axis')
        self.calibration_ax.grid(visible='True')
        self.calibration_canvas.draw()
        self.calibration_label.configure(text=f'Кол-во точек: {len(self.calibration_df)}')

    ### Конец калибровки


if __name__ == "__main__":
    app = App()
    app.mainloop()

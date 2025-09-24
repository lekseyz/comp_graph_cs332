import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider, Button
from PIL import Image
import tkinter as tk
from tkinter import filedialog, messagebox
from tkinter import ttk
import os


def load_image():
    root = tk.Tk()
    root.withdraw()
    
    file_path = filedialog.askopenfilename(
        title="Выберите изображение для HSV редактора",
        filetypes=[("Image files", "*.jpg *.jpeg *.png *.bmp"), ("All files", "*.*")]
    )
    root.destroy()
    
    if file_path and os.path.exists(file_path):
        image = Image.open(file_path)
        if image.mode != 'RGB':
            image = image.convert('RGB')
        max_size = 600
        if image.width > max_size or image.height > max_size:
            image.thumbnail((max_size, max_size), Image.Resampling.LANCZOS)
        return np.array(image)
    else:
        print("Файл не выбран. Создаю тестовое изображение...")
        return create_test_image()


def create_test_image():
    width, height = 400, 300
    image = np.zeros((height, width, 3), dtype=np.uint8)
    
    for x in range(width):
        hue = x / width
        rgb = hsv_to_rgb_single(hue * 360, 1.0, 1.0)
        image[0:50, x] = rgb
    
    for x in range(width):
        sat = x / width
        rgb = hsv_to_rgb_single(180, sat, 1.0)
        image[50:100, x] = rgb
    
    for x in range(width):
        val = x / width
        rgb = hsv_to_rgb_single(0, 1.0, val)
        image[100:150, x] = rgb
    
    image[150:200, 0:80] = [255, 0, 0]
    image[150:200, 80:160] = [0, 255, 0]
    image[150:200, 160:240] = [0, 0, 255]
    image[150:200, 240:320] = [255, 255, 0]
    image[150:200, 320:400] = [255, 0, 255]
    
    image[200:250, 0:100] = [255, 128, 0]
    image[200:250, 100:200] = [0, 255, 255]
    image[200:250, 200:300] = [128, 0, 255]
    image[200:250, 300:400] = [255, 255, 255]
    
    for x in range(width):
        gray_value = int(255 * x / width)
        image[250:300, x] = [gray_value, gray_value, gray_value]
    
    return image


def rgb_to_hsv(rgb_image):
    rgb_norm = rgb_image.astype(np.float32) / 255.0
    
    r = rgb_norm[:, :, 0]
    g = rgb_norm[:, :, 1]
    b = rgb_norm[:, :, 2]
    
    max_val = np.maximum(np.maximum(r, g), b)
    min_val = np.minimum(np.minimum(r, g), b)
    delta = max_val - min_val
    
    hsv = np.zeros_like(rgb_norm)
    
    hsv[:, :, 2] = max_val
    
    mask = max_val != 0
    hsv[:, :, 1][mask] = delta[mask] / max_val[mask]
    
    mask_delta = delta != 0
    
    mask_r = (max_val == r) & mask_delta
    hsv[:, :, 0][mask_r] = 60 * (((g[mask_r] - b[mask_r]) / delta[mask_r]) % 6)
    
    mask_g = (max_val == g) & mask_delta
    hsv[:, :, 0][mask_g] = 60 * (((b[mask_g] - r[mask_g]) / delta[mask_g]) + 2)
    
    mask_b = (max_val == b) & mask_delta
    hsv[:, :, 0][mask_b] = 60 * (((r[mask_b] - g[mask_b]) / delta[mask_b]) + 4)
    
    return hsv


def hsv_to_rgb(hsv_image):
    h = hsv_image[:, :, 0]
    s = hsv_image[:, :, 1]
    v = hsv_image[:, :, 2]
    
    h_i = np.floor(h / 60).astype(int) % 6
    f = (h / 60) - h_i
    
    p = v * (1 - s)
    q = v * (1 - f * s)
    t = v * (1 - (1 - f) * s)
    
    rgb = np.zeros_like(hsv_image)
    
    mask = h_i == 0
    rgb[:, :, 0][mask] = v[mask]
    rgb[:, :, 1][mask] = t[mask]
    rgb[:, :, 2][mask] = p[mask]
    
    mask = h_i == 1
    rgb[:, :, 0][mask] = q[mask]
    rgb[:, :, 1][mask] = v[mask]
    rgb[:, :, 2][mask] = p[mask]
    
    mask = h_i == 2
    rgb[:, :, 0][mask] = p[mask]
    rgb[:, :, 1][mask] = v[mask]
    rgb[:, :, 2][mask] = t[mask]
    
    mask = h_i == 3
    rgb[:, :, 0][mask] = p[mask]
    rgb[:, :, 1][mask] = q[mask]
    rgb[:, :, 2][mask] = v[mask]
    
    mask = h_i == 4
    rgb[:, :, 0][mask] = t[mask]
    rgb[:, :, 1][mask] = p[mask]
    rgb[:, :, 2][mask] = v[mask]
    
    mask = h_i == 5
    rgb[:, :, 0][mask] = v[mask]
    rgb[:, :, 1][mask] = p[mask]
    rgb[:, :, 2][mask] = q[mask]
    
    rgb_uint8 = (rgb * 255).astype(np.uint8)
    
    return rgb_uint8


def hsv_to_rgb_single(h, s, v):
    h = h % 360
    c = v * s
    x = c * (1 - abs((h / 60) % 2 - 1))
    m = v - c
    
    if 0 <= h < 60:
        r, g, b = c, x, 0
    elif 60 <= h < 120:
        r, g, b = x, c, 0
    elif 120 <= h < 180:
        r, g, b = 0, c, x
    elif 180 <= h < 240:
        r, g, b = 0, x, c
    elif 240 <= h < 300:
        r, g, b = x, 0, c
    else:
        r, g, b = c, 0, x
    
    return np.array([(r + m) * 255, (g + m) * 255, (b + m) * 255], dtype=np.uint8)


class HSVEditor:
    
    def __init__(self, image):
        self.original_image = image
        self.original_hsv = rgb_to_hsv(image)
        self.current_image = image.copy()
        
        self.hue_shift = 0
        self.sat_scale = 1.0
        self.val_scale = 1.0
        
        self.setup_matplotlib_gui()
    
    def setup_matplotlib_gui(self):
        self.fig = plt.figure(figsize=(14, 8))
        self.fig.suptitle('Интерактивный HSV редактор', fontsize=16, fontweight='bold')
        
        self.ax_original = plt.subplot(1, 2, 1)
        self.ax_original.set_title('Оригинал')
        self.ax_original.axis('off')
        self.img_original = self.ax_original.imshow(self.original_image)
        
        self.ax_result = plt.subplot(1, 2, 2)
        self.ax_result.set_title('Результат')
        self.ax_result.axis('off')
        self.img_result = self.ax_result.imshow(self.current_image)
        
        plt.subplots_adjust(bottom=0.35)
        
        ax_hue = plt.axes([0.15, 0.20, 0.7, 0.03])
        self.slider_hue = Slider(ax_hue, 'Оттенок (Hue)', -180, 180, valinit=0, 
                                 valstep=1, color='orange')
        
        ax_sat = plt.axes([0.15, 0.15, 0.7, 0.03])
        self.slider_sat = Slider(ax_sat, 'Насыщенность', 0, 2, valinit=1.0, 
                                valstep=0.01, color='green')
        
        ax_val = plt.axes([0.15, 0.10, 0.7, 0.03])
        self.slider_val = Slider(ax_val, 'Яркость (Value)', 0, 2, valinit=1.0, 
                                valstep=0.01, color='blue')
        
        ax_reset = plt.axes([0.2, 0.03, 0.1, 0.04])
        self.btn_reset = Button(ax_reset, 'Сброс', color='lightgray', hovercolor='gray')
        
        ax_save = plt.axes([0.35, 0.03, 0.1, 0.04])
        self.btn_save = Button(ax_save, 'Сохранить', color='lightgreen', hovercolor='green')
        
        ax_compare = plt.axes([0.5, 0.03, 0.15, 0.04])
        self.btn_compare = Button(ax_compare, 'Показать разницу', color='lightblue', hovercolor='blue')
        
        self.slider_hue.on_changed(self.update_image)
        self.slider_sat.on_changed(self.update_image)
        self.slider_val.on_changed(self.update_image)
        self.btn_reset.on_clicked(self.reset_values)
        self.btn_save.on_clicked(self.save_image)
        self.btn_compare.on_clicked(self.show_difference)
        
        plt.show()
    
    def update_image(self, val=None):
        self.hue_shift = self.slider_hue.val
        self.sat_scale = self.slider_sat.val
        self.val_scale = self.slider_val.val
        
        modified_hsv = self.original_hsv.copy()
        
        modified_hsv[:, :, 0] = (modified_hsv[:, :, 0] + self.hue_shift) % 360
        
        modified_hsv[:, :, 1] = np.clip(modified_hsv[:, :, 1] * self.sat_scale, 0, 1)
        
        modified_hsv[:, :, 2] = np.clip(modified_hsv[:, :, 2] * self.val_scale, 0, 1)
        
        self.current_image = hsv_to_rgb(modified_hsv)
        
        self.img_result.set_data(self.current_image)
        
        self.ax_result.set_title(f'H: {self.hue_shift:+.0f}° | S: ×{self.sat_scale:.2f} | V: ×{self.val_scale:.2f}')
        
        self.fig.canvas.draw_idle()
    
    def reset_values(self, event):
        self.slider_hue.set_val(0)
        self.slider_sat.set_val(1.0)
        self.slider_val.set_val(1.0)
    
    def save_image(self, event):
        root = tk.Tk()
        root.withdraw()
        file_path = filedialog.asksaveasfilename(
            defaultextension=".png",
            filetypes=[("PNG", "*.png"), ("JPEG", "*.jpg"), ("All files", "*.*")]
        )
        root.destroy()
        
        if file_path:
            Image.fromarray(self.current_image).save(file_path)
            print(f"Изображение сохранено: {file_path}")
            root = tk.Tk()
            root.withdraw()
            messagebox.showinfo("Успех", f"Изображение сохранено:\n{file_path}")
            root.destroy()
    
    def show_difference(self, event):
        fig_diff = plt.figure(figsize=(15, 5))
        fig_diff.suptitle('Анализ изменений', fontsize=14, fontweight='bold')
        
        ax1 = plt.subplot(1, 4, 1)
        ax1.imshow(self.original_image)
        ax1.set_title('Оригинал')
        ax1.axis('off')
        
        ax2 = plt.subplot(1, 4, 2)
        ax2.imshow(self.current_image)
        ax2.set_title('Результат')
        ax2.axis('off')
        
        diff = np.abs(self.original_image.astype(float) - self.current_image.astype(float))
        ax3 = plt.subplot(1, 4, 3)
        im3 = ax3.imshow(diff.astype(np.uint8))
        ax3.set_title('Абсолютная разница')
        ax3.axis('off')
        
        diff_enhanced = np.clip(diff * 3, 0, 255)
        ax4 = plt.subplot(1, 4, 4)
        im4 = ax4.imshow(diff_enhanced.astype(np.uint8))
        ax4.set_title('Усиленная разница (×3)')
        ax4.axis('off')
        
        plt.tight_layout()
        plt.show()


class TkinterHSVEditor:
    
    def __init__(self, image):
        self.original_image = image
        self.original_hsv = rgb_to_hsv(image)
        self.current_image = image.copy()
        
        self.setup_tkinter_gui()
    
    def setup_tkinter_gui(self):
        self.root = tk.Tk()
        self.root.title("HSV Редактор изображений")
        self.root.geometry("800x600")
        
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        title_label = ttk.Label(main_frame, text="Интерактивный HSV редактор", 
                                font=('Arial', 16, 'bold'))
        title_label.grid(row=0, column=0, columnspan=3, pady=10)
        
        info_frame = ttk.LabelFrame(main_frame, text="Информация", padding="10")
        info_frame.grid(row=1, column=0, columnspan=3, sticky=(tk.W, tk.E), pady=10)
        
        info_text = """
        Оттенок (Hue): Изменяет цвет (красный → зеленый → синий)
        Насыщенность (Saturation): Изменяет чистоту цвета (серый ← → яркий)
        Яркость (Value): Изменяет светлоту (темный ← → светлый)
        """
        ttk.Label(info_frame, text=info_text).pack()
        
        controls_frame = ttk.LabelFrame(main_frame, text="Управление", padding="10")
        controls_frame.grid(row=2, column=0, columnspan=3, sticky=(tk.W, tk.E), pady=10)
        
        self.hue_var = tk.DoubleVar(value=0)
        self.sat_var = tk.DoubleVar(value=1.0)
        self.val_var = tk.DoubleVar(value=1.0)
        
        ttk.Label(controls_frame, text="Оттенок (Hue):").grid(row=0, column=0, sticky=tk.W)
        self.hue_slider = ttk.Scale(controls_frame, from_=-180, to=180, orient=tk.HORIZONTAL,
                                   variable=self.hue_var, command=self.update_preview)
        self.hue_slider.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=10)
        self.hue_label = ttk.Label(controls_frame, text="0°")
        self.hue_label.grid(row=0, column=2)
        
        ttk.Label(controls_frame, text="Насыщенность:").grid(row=1, column=0, sticky=tk.W)
        self.sat_slider = ttk.Scale(controls_frame, from_=0, to=2, orient=tk.HORIZONTAL,
                                   variable=self.sat_var, command=self.update_preview)
        self.sat_slider.grid(row=1, column=1, sticky=(tk.W, tk.E), padx=10)
        self.sat_label = ttk.Label(controls_frame, text="1.00")
        self.sat_label.grid(row=1, column=2)
        
        ttk.Label(controls_frame, text="Яркость (Value):").grid(row=2, column=0, sticky=tk.W)
        self.val_slider = ttk.Scale(controls_frame, from_=0, to=2, orient=tk.HORIZONTAL,
                                   variable=self.val_var, command=self.update_preview)
        self.val_slider.grid(row=2, column=1, sticky=(tk.W, tk.E), padx=10)
        self.val_label = ttk.Label(controls_frame, text="1.00")
        self.val_label.grid(row=2, column=2)
        
        controls_frame.columnconfigure(1, weight=1)
        
        buttons_frame = ttk.Frame(main_frame)
        buttons_frame.grid(row=3, column=0, columnspan=3, pady=10)
        
        ttk.Button(buttons_frame, text="Сброс", command=self.reset_values).pack(side=tk.LEFT, padx=5)
        ttk.Button(buttons_frame, text="Показать результат", command=self.show_result).pack(side=tk.LEFT, padx=5)
        ttk.Button(buttons_frame, text="Сохранить", command=self.save_image).pack(side=tk.LEFT, padx=5)
        ttk.Button(buttons_frame, text="Выход", command=self.root.quit).pack(side=tk.LEFT, padx=5)
        
        self.status_label = ttk.Label(main_frame, text="Готов к работе", relief=tk.SUNKEN)
        self.status_label.grid(row=4, column=0, columnspan=3, sticky=(tk.W, tk.E), pady=5)
        
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(0, weight=1)
        
        self.root.mainloop()
    
    def update_preview(self, value=None):
        hue = self.hue_var.get()
        sat = self.sat_var.get()
        val = self.val_var.get()
        
        self.hue_label.config(text=f"{hue:+.0f}°")
        self.sat_label.config(text=f"{sat:.2f}")
        self.val_label.config(text=f"{val:.2f}")
        
        modified_hsv = self.original_hsv.copy()
        modified_hsv[:, :, 0] = (modified_hsv[:, :, 0] + hue) % 360
        modified_hsv[:, :, 1] = np.clip(modified_hsv[:, :, 1] * sat, 0, 1)
        modified_hsv[:, :, 2] = np.clip(modified_hsv[:, :, 2] * val, 0, 1)
        
        self.current_image = hsv_to_rgb(modified_hsv)
        
        self.status_label.config(text=f"H: {hue:+.0f}° | S: ×{sat:.2f} | V: ×{val:.2f}")
    
    def reset_values(self):
        self.hue_var.set(0)
        self.sat_var.set(1.0)
        self.val_var.set(1.0)
        self.status_label.config(text="Значения сброшены")
    
    def show_result(self):
        self.update_preview()
        
        fig, axes = plt.subplots(1, 2, figsize=(12, 6))
        fig.suptitle('Результат HSV преобразования', fontsize=14, fontweight='bold')
        
        axes[0].imshow(self.original_image)
        axes[0].set_title('Оригинал')
        axes[0].axis('off')
        
        axes[1].imshow(self.current_image)
        axes[1].set_title(f'После изменения\nH: {self.hue_var.get():+.0f}° | S: ×{self.sat_var.get():.2f} | V: ×{self.val_var.get():.2f}')
        axes[1].axis('off')
        
        plt.tight_layout()
        plt.show()
    
    def save_image(self):
        self.update_preview()
        
        file_path = filedialog.asksaveasfilename(
            defaultextension=".png",
            filetypes=[("PNG", "*.png"), ("JPEG", "*.jpg"), ("All files", "*.*")]
        )
        
        if file_path:
            Image.fromarray(self.current_image).save(file_path)
            self.status_label.config(text=f"Сохранено: {os.path.basename(file_path)}")
            messagebox.showinfo("Успех", f"Изображение сохранено:\n{file_path}")


def main():
    print("=" * 60)
    print("ЗАДАНИЕ 3: Интерактивный HSV редактор")
    print("=" * 60)
    
    print("\nЗагрузка изображения...")
    original_image = load_image()
    print(f"Размер изображения: {original_image.shape[1]}×{original_image.shape[0]} пикселей")
    
    print("\nВыберите тип интерфейса:")
    print("1. Matplotlib (рекомендуется) - более наглядный")
    print("2. Tkinter - классический оконный интерфейс")
    
    choice = input("Ваш выбор (1 или 2): ")
    
    if choice == '2':
        print("\nЗапуск Tkinter интерфейса...")
        print("Используйте ползунки для изменения HSV параметров.")
        print("Нажмите 'Показать результат' для просмотра изменений.")
        editor = TkinterHSVEditor(original_image)
    else:
        print("\nЗапуск Matplotlib интерфейса...")
        print("Используйте ползунки для изменения HSV параметров в реальном времени.")
        print("\nПОДСКАЗКИ:")
        print("- Hue: сдвиг цвета по спектру (красный → зеленый → синий)")
        print("- Saturation: 0 = черно-белое, 2 = очень насыщенное")
        print("- Value: 0 = черное, 2 = очень яркое")
        editor = HSVEditor(original_image)
    
    print("\nПрограмма завершена!")


if __name__ == "__main__":
    main()

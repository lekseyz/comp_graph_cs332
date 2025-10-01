import tkinter as tk
from tkinter import colorchooser
import math



class GradientTriangleApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Градиентное окрашивание треугольника")
        
        self.canvas = tk.Canvas(root, width=800, height=600, bg='white')
        self.canvas.pack(side=tk.LEFT)
        
        control_frame = tk.Frame(root)
        control_frame.pack(side=tk.RIGHT, fill=tk.BOTH, padx=10, pady=10)
        
        tk.Label(control_frame, text="Цвета вершин:", font=('Arial', 10, 'bold')).pack(anchor='w', pady=(10,5))
        
        self.color1_btn = tk.Button(control_frame, text="Вершина 1", bg='red', 
                                     command=lambda: self.choose_color(0))
        self.color1_btn.pack(fill=tk.X, pady=2)
        
        self.color2_btn = tk.Button(control_frame, text="Вершина 2", bg='green',
                                     command=lambda: self.choose_color(1))
        self.color2_btn.pack(fill=tk.X, pady=2)
        
        self.color3_btn = tk.Button(control_frame, text="Вершина 3", bg='blue',
                                     command=lambda: self.choose_color(2))
        self.color3_btn.pack(fill=tk.X, pady=2)
        
        tk.Label(control_frame, text="\nДействия:", font=('Arial', 10, 'bold')).pack(anchor='w', pady=(10,5))
        
        self.rasterize_btn = tk.Button(control_frame, text="Растеризовать", 
                                        command=self.rasterize_triangle, state='disabled')
        self.rasterize_btn.pack(fill=tk.X, pady=2)
        
        tk.Button(control_frame, text="Очистить", command=self.clear_canvas).pack(fill=tk.X, pady=2)
        
        self.info_label = tk.Label(control_frame, text="Кликните для добавления вершин", 
                                    wraplength=200, justify='left')
        self.info_label.pack(pady=(20,0))
        
        self.vertices = []
        self.colors = [(255, 0, 0), (0, 255, 0), (0, 0, 255)]
        self.vertex_ids = []
        
        self.canvas.bind('<Button-1>', self.add_vertex)
        
    def rgb_to_hex(self, rgb):
        return '#{:02x}{:02x}{:02x}'.format(rgb[0], rgb[1], rgb[2])
    
    def hex_to_rgb(self, hex_color):
        hex_color = hex_color.lstrip('#')
        return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))
    
    def choose_color(self, vertex_index):
        color = colorchooser.askcolor(initialcolor=self.rgb_to_hex(self.colors[vertex_index]))
        if color[0]:
            self.colors[vertex_index] = tuple(int(c) for c in color[0])
            if vertex_index == 0:
                self.color1_btn.config(bg=color[1])
            elif vertex_index == 1:
                self.color2_btn.config(bg=color[1])
            else:
                self.color3_btn.config(bg=color[1])
    
    def add_vertex(self, event):
        if len(self.vertices) < 3:
            x, y = event.x, event.y
            self.vertices.append((x, y))
            
            color = self.rgb_to_hex(self.colors[len(self.vertices) - 1])
            vertex_id = self.canvas.create_oval(x-5, y-5, x+5, y+5, fill=color, outline='black', width=2)
            self.vertex_ids.append(vertex_id)
            
            if len(self.vertices) == 2:
                self.canvas.create_line(self.vertices[0], self.vertices[1], fill='gray', width=1)
            elif len(self.vertices) == 3:
                self.canvas.create_line(self.vertices[1], self.vertices[2], fill='gray', width=1)
                self.canvas.create_line(self.vertices[2], self.vertices[0], fill='gray', width=1)
                self.rasterize_btn.config(state='normal')
                self.info_label.config(text="Треугольник готов!\nНажмите 'Растеризовать'")
            else:
                self.info_label.config(text=f"Добавлено вершин: {len(self.vertices)}/3")
    
    def calculate_triangle_area(self, p1, p2, p3):
        return ((p2[0] - p1[0]) * (p3[1] - p1[1]) - (p3[0] - p1[0]) * (p2[1] - p1[1])) / 2.0
    
    def get_barycentric_coordinates(self, p, v0, v1, v2):
        area_abc = self.calculate_triangle_area(v0, v1, v2)
        
        if abs(area_abc) < 1e-10:
            return None, None, None
        
        area_pbc = self.calculate_triangle_area(p, v1, v2)
        area_pca = self.calculate_triangle_area(p, v2, v0)
        area_pab = self.calculate_triangle_area(p, v0, v1)
        
        a = area_pbc / area_abc
        b = area_pca / area_abc
        c = area_pab / area_abc
        
        return a, b, c
    
    def interpolate_color(self, a, b, c, color0, color1, color2):
        r = int(a * color0[0] + b * color1[0] + c * color2[0])
        g = int(a * color0[1] + b * color1[1] + c * color2[1])
        b_val = int(a * color0[2] + b * color1[2] + c * color2[2])
        
        r = max(0, min(255, r))
        g = max(0, min(255, g))
        b_val = max(0, min(255, b_val))
        
        return (r, g, b_val)
    
    def rasterize_triangle(self):
        if len(self.vertices) != 3:
            return
        
        v0, v1, v2 = self.vertices
        c0, c1, c2 = self.colors
        
        min_x = int(min(v0[0], v1[0], v2[0]))
        max_x = int(max(v0[0], v1[0], v2[0]))
        min_y = int(min(v0[1], v1[1], v2[1]))
        max_y = int(max(v0[1], v1[1], v2[1]))
        
        self.info_label.config(text="Растеризация...")
        self.root.update()
        
        img = tk.PhotoImage(width=max_x - min_x + 1, height=max_y - min_y + 1)
        
        for y in range(min_y, max_y + 1):
            for x in range(min_x, max_x + 1):
                a, b, c = self.get_barycentric_coordinates((x, y), v0, v1, v2)
                
                if a is None:
                    continue
                
                if a >= 0 and b >= 0 and c >= 0:
                    color = self.interpolate_color(a, b, c, c0, c1, c2)
                    color_hex = self.rgb_to_hex(color)
                    
                    img.put(color_hex, (x - min_x, y - min_y))
        
        self.canvas.create_image(min_x, min_y, image=img, anchor='nw')
        self.canvas.image = img
        
        for vertex_id in self.vertex_ids:
            self.canvas.tag_raise(vertex_id)
        
        self.info_label.config(text="Растеризация завершена!")
    
    def clear_canvas(self):
        self.canvas.delete('all')
        self.vertices = []
        self.vertex_ids = []
        self.rasterize_btn.config(state='disabled')
        self.info_label.config(text="Кликните для добавления вершин")



if __name__ == '__main__':
    root = tk.Tk()
    app = GradientTriangleApp(root)
    root.mainloop()

import matplotlib.pyplot as plt
import numpy as np

# Данные
threads = list(range(1, 50))
times = [
    19.73, 11.33, 7.83, 5.87, 5.41, 6.06, 5.17, 5.21, 5.13, 5.00,
    5.06, 5.00, 4.78, 4.82, 4.73, 4.74, 4.77, 4.64, 4.70, 4.64,
    4.68, 4.65, 4.62, 4.59, 4.66, 4.54, 4.50, 4.55, 4.53, 4.47,
    4.47, 4.48, 4.52, 4.46, 4.45, 4.42, 4.45, 4.42, 4.38, 4.41,
    4.39, 4.36, 4.36, 4.36, 4.36, 4.35, 4.35, 4.34, 4.33
]

# Создание графика
plt.figure(figsize=(12, 8))

# Основной график
plt.subplot(2, 1, 1)
plt.plot(threads, times, 'bo-', linewidth=2, markersize=4, label='Время выполнения')
plt.xlabel('Количество потоков')
plt.ylabel('Время выполнения (с)')
plt.title('Зависимость времени выполнения от количества потоков')
plt.grid(True, alpha=0.3)
plt.legend()

# Добавление горизонтальной линии для минимального времени
min_time = min(times)
plt.axhline(y=min_time, color='r', linestyle='--', alpha=0.7,
           label=f'Минимальное время: {min_time:.2f} с')

# Увеличенный масштаб для области насыщения
plt.subplot(2, 1, 2)
plt.plot(threads[10:], times[10:], 'go-', linewidth=2, markersize=4,
         label='Время выполнения (11+ потоков)')
plt.xlabel('Количество потоков')
plt.ylabel('Время выполнения (с)')
plt.title('Область насыщения (увеличенный масштаб)')
plt.grid(True, alpha=0.3)
plt.legend()

plt.tight_layout()
plt.show()

# Дополнительный анализ
print(f"Минимальное время: {min_time:.2f} с")
print(f"Максимальное время: {max(times):.2f} с")
print(f"Ускорение (1 поток / минимальное время): {times[0]/min_times:.2f}x")

# График ускорения
speedup = [times[0] / t for t in times]
ideal_speedup = threads

plt.figure(figsize=(10, 6))
plt.plot(threads, speedup, 'ro-', linewidth=2, markersize=4, label='Фактическое ускорение')
plt.plot(threads, ideal_speedup, 'k--', alpha=0.5, label='Идеальное ускорение')
plt.xlabel('Количество потоков')
plt.ylabel('Коэффициент ускорения')
plt.title('Зависимость ускорения от количества потоков')
plt.grid(True, alpha=0.3)
plt.legend()
plt.show()

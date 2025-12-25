import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('results.csv', header=None, names=['n_threads', 'time'])

fig, axes = plt.subplots(2, 2, figsize=(14, 10))

# Весь диапазон
axes[0, 0].plot(df['n_threads'], df['time'], marker='o', color='b')
axes[0, 0].set_title('Все данные')
axes[0, 0].set_xlabel('n')
axes[0, 0].set_ylabel('Время (сек)')
axes[0, 0].grid(True)

# Только малые n
axes[0, 1].plot(df[df['n_threads'] <= 32]['n_threads'], df[df['n_threads'] <= 32]['time'], marker='o', color='r')
axes[0, 1].set_title('n <= 32')
axes[0, 1].set_xlabel('n')
axes[0, 1].set_ylabel('Время (сек)')
axes[0, 1].grid(True)

# Только средние n
axes[1, 0].plot(df[(df['n_threads'] > 32) & (df['n_threads'] <= 256)]['n_threads'],
                df[(df['n_threads'] > 32) & (df['n_threads'] <= 256)]['time'], marker='s', color='g')
axes[1, 0].set_title('32 < n <= 256')
axes[1, 0].set_xlabel('n')
axes[1, 0].set_ylabel('Время (сек)')
axes[1, 0].grid(True)

# Только большие n
df_large = df[df['n_threads'] > 500]
if not df_large.empty:
    axes[1, 1].plot(df_large['n_threads'], df_large['time'], marker='^', color='orange')
    axes[1, 1].set_title('n > 500')
    axes[1, 1].set_xlabel('n')
    axes[1, 1].set_ylabel('Время (сек)')
    axes[1, 1].grid(True)
else:
    axes[1, 1].text(0.5, 0.5, 'Нет данных', ha='center', va='center', transform=axes[1, 1].transAxes)
    axes[1, 1].set_title('n > 500 (нет данных)')

plt.tight_layout()
plt.savefig('performance_subplots.png')
plt.show()

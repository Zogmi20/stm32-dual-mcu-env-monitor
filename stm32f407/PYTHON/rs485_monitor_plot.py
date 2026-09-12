import serial
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
from collections import deque
import time

# 设置中文字体
plt.rcParams['font.sans-serif'] = ['Microsoft YaHei', 'SimHei', 'Arial Unicode MS']
plt.rcParams['axes.unicode_minus'] = False

# ====================== 【高级工控暗黑透明风格全局配置】 ======================
# 画布背景黑色
plt.rcParams['figure.facecolor'] = '#0c0c12'
# 坐标轴区域背景黑色
plt.rcParams['axes.facecolor'] = '#0c0c12'
# 坐标轴文字、刻度浅灰白
plt.rcParams['text.color'] = '#e6e6e6'
plt.rcParams['xtick.color'] = '#cccccc'
plt.rcParams['ytick.color'] = '#cccccc'
# 图例背景半透明
plt.rcParams['legend.framealpha'] = 0.25
plt.rcParams['legend.facecolor'] = '#222222'
plt.rcParams['legend.labelcolor'] = '#eeeeee'

# --------------------------配置--------------------------
SERIAL_PORT = 'COM9'
BAUDRATE = 115200
MAX_POINTS = 100

temp_data = deque(maxlen=MAX_POINTS)
humi_data = deque(maxlen=MAX_POINTS)
time_data = deque(maxlen=MAX_POINTS)

print("=" * 50)
print("🌡️  RS485 温湿度实时监控")
print("=" * 50)

try:
    ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
    ser.reset_input_buffer()
    print(f"✅ Connected to {SERIAL_PORT}\n")
except Exception as e:
    print(f"❌ Error: {e}")
    exit(1)

plt.ion()
fig, ax1 = plt.subplots(figsize=(12, 6))
fig.canvas.manager.set_window_title('RS485 温湿度监控')

# 创建右侧Y轴，只创建一次！！！
ax2 = ax1.twinx()

# 预先画两条空曲线，高级配色 + 轻微透明度
# 温度：暖橙红；湿度：青蓝色；alpha 线条半透明，发光柔和
line1, = ax1.plot([], [], color='#ff6b35', linewidth=2.8, alpha=0.88, label="温度 (°C)")
line2, = ax2.plot([], [], color='#38b6ff', linewidth=2.8, alpha=0.88, label="湿度 (%)")

# 固定坐标轴范围、标签、网格，只初始化一次
ax1.set_xlabel('时间 (秒)', fontsize=14, color='#e6e6e6')
ax1.set_ylabel('温度 (°C)', color='#ff6b35', fontsize=14)
ax1.tick_params(axis='y', labelcolor='#ff6b35')
# 网格 低透明度，暗色网格不刺眼
ax1.grid(True, alpha=0.18, color="#777777", linestyle='-')
ax1.set_ylim(20, 45)

ax2.set_ylabel('湿度 (%)', color='#38b6ff', fontsize=14)
ax2.tick_params(axis='y', labelcolor='#38b6ff')
ax2.set_ylim(0, 100)

# 合并图例（半透明效果）
lines = [line1, line2]
ax1.legend(lines, [l.get_label() for l in lines], loc='upper right', fontsize=14)

fig.show()

count = 0
start_time = time.time()

print("⏳ 实时监控中...\n")

while True:
    try:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if not line:
            continue

        if 'T:' in line and 'H:' in line:
            parts = line.split(',')
            temp = float(parts[0].split(':')[1])
            humi = float(parts[1].split(':')[1].strip())

            count += 1
            current_time = time.time() - start_time

            temp_data.append(temp)
            humi_data.append(humi)
            time_data.append(current_time)

            print(f"[{count}] 🌡️ {temp:.1f}°C  💧 {humi:.1f}%  ⏱️ {current_time:.1f}s")

            # ========== 核心更新曲线 ==========
            line1.set_data(list(time_data), list(temp_data))
            line2.set_data(list(time_data), list(humi_data))

            # 自动适配X轴范围
            ax1.relim()
            ax1.autoscale_view()

            # 更新标题，浅白色字体
            ax1.set_title(f'实时温湿度  |  温度: {temp:.1f}°C  湿度: {humi:.1f}%',
                          fontsize=16, color="#eeeeee")

            fig.canvas.draw()
            fig.canvas.flush_events()
            plt.pause(0.01)

    except KeyboardInterrupt:
        print("\n用户中断")
        break
    except Exception as e:
        print(f"Error: {e}")
        time.sleep(0.1)

ser.close()
plt.ioff()
plt.show()

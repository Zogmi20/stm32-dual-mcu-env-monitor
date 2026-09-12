# STM32 双 MCU 温湿度采集与监控系统

基于 **STM32F103 + STM32F407** 的双节点温湿度监控系统。F103 作采集节点，裸机驱动 DHT11 采集温湿度经 UART 上报；F407 作处理节点，运行 FreeRTOS 完成显示、报警、存储与远传，并配套 Python 上位机实现运行状态的实时可视化。

## 功能

- **采集**：DHT11 温湿度采集，独立节点周期上报
- **显示**：TFTLCD 实时刷新
- **报警**：阈值可通过按键设定，越限蜂鸣提示
- **存储**：SD 卡 CSV 记录，基于 FATFS 文件系统
- **远传**：RS485 转发，采用 DMA 搬运数据以降低 CPU 占用
- **守护**：IWDG 看门狗，任一任务卡死自动复位
- **上位机**：Python + matplotlib 绘制温湿度实时曲线

## 硬件

| 节点 | MCU | 职责 |
| --- | --- | --- |
| 采集节点 | STM32F103 | 驱动 DHT11 周期采集温湿度，经 UART 上报 |
| 处理节点 | STM32F407 | LCD 显示、越限报警、按键设阈值、Flash 参数保存、RS485 转发、SD 卡记录 |

- 温湿度传感器：DHT11（单总线时序驱动）
- 显示：TFTLCD
- 存储：SD 卡
- 通信：UART（板间）、RS485（远传）

## 软件架构

### 采集节点（F103，裸机）

DHT11 单总线时序驱动 → 周期读取温湿度 → UART 发送。程序结构简单，不使用 RTOS。

### 处理节点（F407，FreeRTOS）

抽象为 3 个任务，以 2 条消息队列解耦：

| 任务 | 职责 |
| --- | --- |
| 采集解析 | 接收 F103 上报的数据，解析后投递至队列 |
| LCD 显示 | 从队列取数据刷新显示 |
| RS485 + SD 记录 | 从队列取数据写入 CSV 并经 RS485 转发 |

- 以 2 条消息队列构建**生产者-消费者模型**，实现采集与显示、存储的解耦
- 消息由**消息池**统一分配，**双缓冲**防止消息被覆盖
- 使用**互斥锁**保护 LCD 这一共享资源

### 可靠性设计

- **按键消抖**：PA0 采用 EXTI 中断 + 松手确认 + 20 ms 软件消抖，规避机械抖动导致的误触发
- **看门狗守护**：各任务周期性向 IWDG 报到，任一任务卡死即自动复位
- **参数持久化**：运行阈值写入 Flash，掉电不丢失

## 仓库结构

两个节点各自是一份独立的 Keil 工程，分别放在对应目录下：

```
.
├── stm32f103/                     # 采集节点（裸机）
│   ├── Core/                      # CubeMX 初始化代码与 main，含 DHT11 驱动
│   ├── Drivers/                   # STM32 HAL 驱动
│   ├── MDK-ARM/                   # Keil 工程（STM32F103.uvprojx）
│   └── STM32F103.ioc              # CubeMX 工程配置
├── stm32f407/                     # 处理节点（FreeRTOS）
│   ├── BSP/                       # 板级驱动（DHT11 / SD / RS485 / 报警等）
│   ├── Core/                      # CubeMX 初始化代码与 main
│   ├── Drivers/                   # STM32 HAL 驱动
│   ├── FATFS/                     # FATFS 文件系统
│   ├── Middlewares/Third_Party/   # FreeRTOS
│   ├── SYSTEM/                    # 系统层
│   ├── PYTHON/                    # Python 上位机（rs485_monitor_plot.py）
│   ├── MDK-ARM/                   # Keil 工程（Project.uvprojx）
│   └── Project.ioc                # CubeMX 工程配置
├── .gitignore
└── README.md
```

## 编译与运行

**采集节点（stm32f103）**

1. Keil MDK 打开 `stm32f103/MDK-ARM/STM32F103.uvprojx`
2. 编译并下载至 STM32F103 开发板
3. 上电后驱动 DHT11 周期采集温湿度，经 UART 上报

**处理节点（stm32f407）**

1. Keil MDK 打开 `stm32f407/MDK-ARM/Project.uvprojx`
2. 编译并下载至 STM32F407 开发板
3. 两节点分别上电，F407 接收 F103 上报的数据
4. LCD 显示实时数据，可按 PA0 按键调整报警阈值

> 两个节点的引脚与外设分配分别见 `STM32F103.ioc` 与 `Project.ioc`，可用 STM32CubeMX 打开查看。

## 上位机

`stm32f407/PYTHON/rs485_monitor_plot.py` 通过串口读取下位机转发的数据，用双 Y 轴实时曲线显示温度与湿度。

```bash
pip install pyserial matplotlib
python stm32f407/PYTHON/rs485_monitor_plot.py
```

运行前按实际接线修改脚本开头的两个参数：

```python
SERIAL_PORT = 'COM9'     # 串口号
BAUDRATE    = 115200     # 波特率，需与下位机一致
```

脚本按 `T:25.3,H:60.5` 的格式解析每一行上报，窗口内保留最近 100 个采样点（`MAX_POINTS`）滚动刷新。

## 说明

- 本项目为个人学习实践，双 MCU 分工与 FreeRTOS 任务划分均已实际调试通过。
- 温湿度采样周期、消息队列长度、任务优先级等参数见源码中各任务的配置。

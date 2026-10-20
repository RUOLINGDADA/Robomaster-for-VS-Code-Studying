<#
.SYNOPSIS
从 Core 源码生成宏参考和关键函数摘录。
.DESCRIPTION
只写 doc/MACRO_REFERENCE.md 和 doc/IMPLEMENTATION_GUIDE.md。
条件分支原样保留；源码表达式不代表编译器 -D 覆盖后的生效值。
#>
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$docRoot = Join-Path $projectRoot 'doc'
$utf8 = [Text.UTF8Encoding]::new($false)
$newline = [Environment]::NewLine
$fence = ([string][char]96) * 3
$cache = @{}
Get-ChildItem (Join-Path $projectRoot 'Core') -Recurse -File |
  Where-Object { $_.Extension -in '.c','.h' } | ForEach-Object {
    $path = $_.FullName.Substring($projectRoot.Length + 1).Replace('\','/')
    $cache[$path] = [IO.File]::ReadAllText($_.FullName)
  }
function Get-MaskedSource([string]$source) {
  # 保留字符数和换行，屏蔽字符串/注释的伪大括号和伪宏定义。
  [regex]::Replace($source, '/\*[\s\S]*?\*/|//[^\r\n]*|"(?:\\.|[^"\\])*"|''(?:\\.|[^''\\])*''',
    [Text.RegularExpressions.MatchEvaluator]{
      param($match)
      [regex]::Replace($match.Value, '[^\r\n]', ' ')
    })
}
function Get-Mechanism([string]$name) {
  switch -Regex ($name) {
    '_H$' { return '包含保护：避免同一翻译单元重复声明，没有控制算法。' }
    '^INS_MAG_ENABLE$' { return '0/1磁采集总门；0跳过IST事务、DRDY通知及I2C/IST中断硬件检查，并强制有效磁融合许可为0，保留标定参数。MotionCal须同时开采集。' }
    '^(YAW|PITCH)_STABILIZATION_ENABLE$' { return '0/1正式反馈选择；1使用六轴惯性姿态，未就绪/无效则捕获编码器保持并暂停人工输入；固定机械角测试始终绕过INS。' }
    '^(YAW|PITCH)_IMU_FEEDBACK_SIGN$' { return '±1惯性角/欧拉角速度到编码器方向映射，当前Yaw=-1、Pitch=+1；不替代电流/编码器测速符号。' }
    '^(YAW|PITCH)_IMU_READY_MS$' { return 'HAL ms，>0且大于新鲜度期限，当前100；READY有效且样本持续更新满足窗口后捕获当时姿态，只在切换边沿锁定。' }
    '^(YAW|PITCH)_IMU_TIMEOUT_MS$' { return 'HAL ms，>0且小于资格窗口，当前10；陀螺/加计任一年龄超过期限即捕获编码器保持，不撤销正常CAN闭环许可。' }
    '^(YAW|PITCH)_IMU_EXIT_PITCH_DEG$' { return 'deg，0<值<90，当前80；绝对Pitch达到阈值转编码器保持，避免ZYX测速除cos(Pitch)接近零。' }
    '^(YAW|PITCH)_IMU_ENTER_PITCH_DEG$' { return 'deg，0<值<退出阈值，当前75；编码器保持时回到该内区且连续有效更新后才能重入惯性反馈。' }
    '^GIMBAL_AXIS_(DEG_TO_RAD|RAD_S_TO_RPM|RAW_PER_DEG)$' { return 'deg→rad、rad/s→rpm及deg→等效count换算；ZYX陀螺投影与姿态保持共享单位，不能直接使用机体gyroZ作为Yaw速度。' }
    '^INS_MOUNT_CONFIRMED$' { return '无量纲0/1发布门；0强制姿态无效，1允许通过READY和数据检查的姿态供使用或方向复测。本开关不自动证明物理三轴验收，当前实测状态见INS模块。' }
    '^INS_BMI_ROTATION$' { return 'BMI088芯片→云台前右下的无量纲正交矩阵，须det=+1；同时转换比力和角速度，实测来源及待复核方向见INS模块。' }
    '^INS_MAG_ROTATION$' { return 'IST8310芯片→云台前右下的无量纲正交矩阵，须det=+1；磁标定后使用，必须独立确认，不能照抄BMI088。' }
    '^INS_MAG_(CALIBRATED|BIAS|SOFTIRON)' { return '先在磁芯片坐标减μT硬铁偏置并乘对称软铁矩阵，再安装旋转；有效磁许可还要求INS_MAG_ENABLE=1且非MotionCal模式，当前采集关闭时仅保留参数。' }
    '^INS_MAG_CALIBRATION_PERIOD_MS$' { return 'MotionCal原始磁帧的最小发送尝试间隔，HAL ms、>=100；新鲜且未重复才发，忙时丢帧，不补发队列。' }
    '^INS_MOTIONCAL_COUNTS_PER_LSB$' { return 'IST8310 0.3μT/LSB转MotionCal 0.1μT/count，原始有符号值先扩展再乘3；越出int16范围整帧拒绝。' }
    '^INS_STILL' { return '连续静止窗口的时间、角速度/比力和每轴方差门；运动/过期/方差不合格重开，完成后估计机体系零偏。' }
    '^INS_(AHRS|GRAVITY|ACCEL_(FULL|REJECT)|MAG_(MIN|MAX|RELATIVE|INNOVATION|RATE|RECOVERY)|MAX_DT)' { return 'AHRS实际dt积分、重力降权及磁模长/创新/速率/恢复门；精确单位、范围与副作用见宏原文和INS说明。' }
    '^INS_(GYRO_TIMEOUT|ACCEL_TIMEOUT|MAG_TIMEOUT|DMA_TIMEOUT|SPI_TIMEOUT|I2C_TIMEOUT|RETRY|TEMPERATURE|LOG|EVENT|MAG_POLL)' { return 'INS采集新鲜度、事务超时、重试、事件或日志周期；HAL ms与FreeRTOS Tick分别使用，不能互相相减。' }
    '^INS_(ACCEL_RANGE|ACCEL_CONF|GYRO_RANGE|GYRO_BANDWIDTH)' { return 'BMI088量程/滤波/ODR的寄存器编码；改变后须重新核对物理量、DRDY频率与执行预算。' }
    '^(BMI088_|IST8310_|AHRS_|INS_RAD_TO_DEG)' { return '惯性/磁驱动的协议常量、单位换算或数值保护；来源和单位见宏注释，接口与状态机见INS说明。' }
    '^CHASSIS_WHEEL_COUNT$' { return '四轮数组容量，顺序为CAN1 ID1右前、ID2左前、ID3左后、ID4右后。' }
    '^CHASSIS_ID[1-4]_SIGN$' { return '安装符号±1，同时变换本轮反馈rpm与发送电流；不是两个独立符号。' }
    '^CHASSIS_STICK_DEADBAND_RAW$' { return '严格-阈值<raw<阈值归零；外部raw/660×MAX_SPEED_RPM，不扣死区宽度。' }
    '^CHASSIS_MAX_SPEED_RPM$' { return '三轴与各轮目标限幅，转子rpm；逐轮混控后钳位，不整体归一化。' }
    '^CHASSIS_SPEED_KI$' { return '底盘梯形积分增量Ki×(e+e_prev)/2×dt_s/(1+a×abs(e))，单位raw。' }
    '^CHASSIS_INTEGRAL_LIMIT_RAW$' { return '底盘四个独立速度PID的积分raw限幅；总电流另行限幅。' }
    '^CHASSIS_INTEGRAL_ERROR_SCALE$' { return '变速积分a，1/rpm；权重1/(1+a×abs(e))，0禁用权重。' }
    '^CHASSIS_CURRENT_LIMIT_RAW$' { return '速度PID实际限流±4915 raw约6A；不得超过C620协议±16384 raw。' }
    '^CHASSIS_KEYBOARD_T1_MS$' { return '键盘曲线直线段结束时刻ms，届时达到LINEAR_RATIO；配置换算为秒。' }
    '^CHASSIS_KEYBOARD_T3_MS$' { return '曲线达到终点的时刻ms，反向先用同样时长过零；松键立即零目标。' }
    '^CHASSIS_KEYBOARD_LINEAR_RATIO$' { return 'T1时的过渡幅度比例(0,1)，改变起步斜率和对数接入修正。' }
    '^CHASSIS_KEYBOARD_LOG_SHAPE$' { return '平滑对数段形状系数>0；两端三次修正保持值与斜率衔接，须复核单调性。' }
    '^CHASSIS_COMMAND_TIMEOUT_MS$' { return '原始HAL接收时间戳年龄>100ms，底盘回STOP并清键/历史、四轮零电流。' }
    '^CHASSIS_FEEDBACK_TIMEOUT_MS$' { return '真实反馈HAL年龄>100ms，仅停止该轮并清其PID，发送入口再次复核。' }
    '^CHASSIS_CONTROL_GAP_MS$' { return 'HAL实际间隔为0或超过阈值则四轮零并清历史，正常间隔换秒给PID。' }
    '^CHASSIS_TASK_PERIOD_MS$' { return '底盘vTaskDelayUntil调度周期，经pdMS_TO_TICKS转换；PID使用实际HAL间隔。' }
    '^C620_M3508_CURRENT_LIMIT_RAW$' { return 'C620协议电流raw边界±16384约20A；与底盘实际限流分开。' }
    '^C620_M3508_(FEEDBACK_BASE_ID|CONTROL_LOW_ID|CONTROL_HIGH_ID)$' { return 'C620标准反馈0x200+ID；ID1~4控制0x200、ID5~8控制0x1FF，每槽大端2字节。' }
    '^C620_M3508_FRAME_BYTES$' { return 'C620标准数据帧长度8字节；帧校验与负载数组使用。' }
    '^C620_M3508_MAX_DEVICES$' { return '静态注册容量8个；同总线ID与句柄地址不能重复。' }
    'POSITION_KP' { return '位置P：位置误差count乘增益得到rpm（供弹得到电流raw）；见总文档§5/6/7。' }
    'POSITION_KI' { return '位置积分：Ki×误差×dt_s；目标速度饱和且误差向外时恢复上一积分。' }
    'POSITION_KD' { return '位置阻尼：Kd×(命令rpm−滤波rpm)×8192/60。' }
    'POSITION_INTEGRAL' { return '外环积分限制在±该rpm；不是总目标速度上限。' }
    'SPEED_KP' { return '速度P：Kp×(目标rpm−滤波rpm)，输出raw。' }
    'SPEED_KI' { return '速度I：Ki×速度误差×dt_s；候选积分限幅和条件保存。' }
    'SPEED_KD' { return '速度D：Kd×(本误差−上一误差)/dt_s；首帧微分为零。' }
    'SPEED_INTEGRAL' { return '速度PID积分历史限幅，不能替代补偿合成后的总电流限幅。' }
    'FILTER_ALPHA' { return '一阶低通y←y+alpha×(x−y)；权重越小响应越滞后。' }
    'FEED_MOTOR_SNAIL_RAMP_TIME_MS' { return '正式C615启动速率=(活动−停止)×1000/RAMP_MS；Shift释放和故障直接停止，不等待Ramp。' }
    'SLEW|RAMP_TIME' { return 'Ramp变化最多rate×dt_s；PWM速率=(活动−停止)×1000/RAMP_MS。' }
    'VELOCITY_FEEDFORWARD' { return '位置外环输出叠加Kff×有效命令rpm；边界向外命令先归零。' }
    'GRAVITY.*BIAS' { return '余弦补偿常值B，模型sign×(B+A×cos(theta))。' }
    'GRAVITY.*AMPLITUDE' { return '余弦补偿幅值A，要求B+A≤补偿模型电流上限。' }
    'GRAVITY.*TWO_PI' { return '编码器角差×2π/8192转换为余弦输入rad。' }
    'GRAVITY.*ENABLE' { return '补偿开关，关闭返回0；禁用模型也校验参数。' }
    'GRAVITY.*COUNTS_PER_REV' { return '补偿角差int64求差后按8192取余，不改写控制目标。' }
    'GRAVITY.*CURRENT_SIGN' { return '补偿在控制器坐标中的方向；合成后仍乘最终轴电流方向。' }
    'MOUSE.*GAIN' { return '新帧：v←clamp(v+相对位移×浮点增益)，单位‰/count，小数保留；增益分别取各轴配置，增大会提高鼠标灵敏度。' }
    'MOUSE.*HOLD' { return '本轴最后非零事件后保持H毫秒，零位移和另一轴输入不续期。' }
    'MOUSE.*DECAY' { return '回中v=v0×(D−(age−H))/D；age≥H+D归零。' }
    'MOUSE.*OUTPUT_LIMIT' { return '虚拟鼠标单独限幅±L‰，当前100；满输入40 rpm时鼠标目标速度最高4 rpm。摇杆合成后再限幅±1000‰，不限制闭环纠偏。' }
    'FEED_MOTOR_CURRENT_SIGN' { return '供弹协议电流=sign(STEP_COUNTS)×FEED_CURRENT_RAW×CURRENT_SIGN；本符号取±1，不能代替步长方向。' }
    'SIGN' { return '方向映射±1；输入、反馈和电流独立。C615只记录方向，不用PWM切换正反转。' }
    'STEP_COUNTS' { return '每次供弹的电机转子连续count增量；目标加法先检查int64边界，符号决定目标比较和逻辑电流方向；等长位移提供反向反馈兜底。' }
    'STALL_COUNTS' { return '单发停滞窗口：连续角度距最近明显移动位置不超过该count；超过则更新参考位置并清零确认计时。' }
    'STALL_CONFIRM_MS' { return '单发位置停滞连续保持该毫秒数后结束C610上弹并开始发射保持；C615已预旋，不负责卡弹超时保护。' }
    'SINGLE_FIRE_HOLD_MS' { return 'C610上弹停滞确认完成后，保持两路C615活动PWM该毫秒数；计时结束进入等待松键或连发。' }
    'STEP_PAUSE_MS' { return '每步到位周期输出零并记录完成Tick；直到Tick差达到pdMS_TO_TICKS(本停顿)且长按达阈值才建立下一目标。' }
    'CONTINUOUS_PRESS_MS' { return '从左键按下沿计FreeRTOS Tick差；达到后仍需每步到位后的零电流停顿结束才能重复。' }
    'FEED_CURRENT_RAW' { return 'STEP阶段固定M2006电流幅值raw；乘步长符号和CURRENT_SIGN得到协议电流。' }
    'DEADBAND' { return '摇杆|raw|≤阈值输出0；外部按raw×1000/660缩放，不减死区宽度。' }
    'CHANNEL_CENTER' { return '11位通道解包后减1024，得到有符号摇杆偏移。' }
    'CHANNEL_SPAN' { return '摇杆范围校验±660，raw×1000/660转换为千分比。' }
    'MAX_COMMAND_SPEED' { return '输入p/1000×Vmax为rpm，目标count积分增量rpm×8192/60×dt_s。' }
    'MAX_SPEED_TARGET' { return '外环目标速度限幅；位置P变大无法突破该rpm上限。' }
    'ANGLE_LOOP_TARGET' { return '固定目标中心count+deg×8192/360；转整数后检查标定范围。' }
    'CALIBRATION.*(CENTER|MIN|MAX)_ANGLE' { return '连续count：中心作为首帧/补偿参考；min/max作为软件边界。' }
    'CALIBRATION_VALID' { return '标定门要求valid且min<center<max，否则零输出。' }
    'ANGLE_LIMIT_DISABLED' { return 'INT32极值哨兵代表几何边界禁用，不是实测挡块。' }
    'MODE|TEST_ENABLE' { return '编译期正式/测试路径选择，模式不在运行中切换；互斥限制见源#if。' }
    'FEEDBACK_TIMEOUT|OFFLINE_TIMEOUT' { return 'HAL同源年龄≥期限离线，先检查真实首帧；发送前重新判断反馈新鲜度。' }
    'COMMAND_TIMEOUT' { return '真实接收时间戳过期：云台冻结目标保持，供弹清除当前状态并输出零电流/停止脉宽。' }
    'INIT_RETRY|RETRY_PERIOD' { return '初始化/恢复失败节流，比较HAL年龄或转换Tick后等待。' }
    'LOG_PERIOD|PROMPT_PERIOD' { return '诊断/提示限频，通常成功提交后续期，不参与电流控制。' }
    'TASK_PERIOD' { return '绝对唤醒周期；云台按固定配置dt，供弹正式路径测实际Tick差。' }
    'HOLD_MS|STARTUP_STOP_TIME|UP_TIME|DOWN_TIME|STOP_TIME|RUN_TIME' {
      return '测试非阻塞阶段时长；TEST_RUN_TIME=0表示持续活动。'
    }
    'PULSE_US' { return 'C615脉宽us；停止/活动/范围各有独立参数，1MHz时CCR数字等于us。' }
    'CURRENT_RAW|MAX_CURRENT' { return '电流raw：C610协议±10000，GM6020±16384；任务进一步限流或指定测试电流。' }
    'ENCODER_COUNTS' { return '每圈编码器count：最短回绕阈值为一半，rpm换count/s用counts/60。' }
    'CONTROL_ID|FEEDBACK_ID|FEED_MOTOR_ID' { return 'CAN路由/槽位；GM6020反馈0x204+ID，C610反馈0x200+ID。' }
    'FRAME_DLC|FRAME_LENGTH' { return 'CAN帧8字节/DBUS帧18字节长度检查、数组和DMA传输长度。' }
    'FRAME_MAILBOX_LENGTH' { return '环形数组16槽，保留一个空槽区分满/空，最多15帧；满时丢新帧计数。' }
    'DEVICE_ID|MAX_DEVICE_COUNT' { return '协议编号/静态注册表容量，重复注册或非法编号拒绝。' }
    'DBUS_SWITCH' { return '拨杆编码上/中/下=1/3/2，当前不参与使能。' }
    'LOG_TX_BUFFER_SIZE' { return '唯一DMA缓冲区768字节，允许1~767字节；超长整条拒绝。' }
    'GIMBAL_LOG_FORMAT|GIMBAL_LOG_ARGS' { return '统一日志格式/同周期快照字段；轴参数多次展开，须无副作用。' }
    'LOG_ENABLED_CATEGORY_MASK|LOG_CATEGORY_ENABLED|LOG_TRY_PRINTF|LOG_SWITCH_VALID' {
      return '分类位掩码、0/1开关校验和条件运算；关闭分类不求值格式参数。'
    }
    '^LOG_INS_MAG_CALIBRATION_ENABLE$' { return '0/1磁标定模式；1时分类独占MotionCal Raw帧并暂禁磁反馈，不暂停电机任务；需日志总门/USART1门及115200/8N1。' }
    '^LOG_DBUS_TRACE_ENABLE$' { return '0/1输入采集分类开关；开启时独占日志带宽，MotionCal优先；不暂停设备控制。' }
    '^LOG_' { return '编译期日志开关/图表前缀，仅控制诊断，不控制电机使能。' }
    '^DBUS_TRACE_CAPACITY$' { return '16~512条任务私有采集记录；满时丢新诊断记录并计数，不改变真实输入。' }
    '^DBUS_TRACE_BATCH_BYTES$' { return '256~767字节文本暂存，必须小于共用日志DMA缓冲；整行组成批次，接受后才出队。' }
    '^DBUS_TRACE_HEARTBEAT_MS$' { return 'HAL ms，正值且小于半个计数周期；无变化时心跳，命令变化仍逐任务周期观察。' }
    'FREERTOS_FAULT_TASK_NAME_LENGTH' { return '故障任务名缓冲16字节，复制保留末尾NUL。' }
    'config|INCLUDE_|USE_FreeRTOS|vPort|xPort|CMSIS_device|USE_CUSTOM_SYSTICK' {
      return 'FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。'
    }
    'HSE_VALUE|HSI_VALUE|LSE_VALUE|LSI_VALUE|EXTERNAL_CLOCK_VALUE' {
      return '时钟频率Hz；PLL、总线与外设波特率/计时基于这些值。'
    }
    'HSE_STARTUP|LSE_STARTUP|VDD_VALUE|TICK_INT_PRIORITY|USE_RTOS' {
      return 'HAL平台初始化/时基参数，按源注释单位和HAL消费路径解释。'
    }
    'HAL_.*MODULE_ENABLED|USE_HAL_.*CALLBACKS' { return 'HAL驱动编译/回调选项，不意味着项目已有该设备业务。' }
    '^PHY_|^ETH_|^MAC_|^USE_SPI_CRC' { return 'HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。' }
    '^assert_param' { return 'HAL参数断言，USE_FULL_ASSERT分支调用assert_failed，否则空操作。' }
    '^VECT_TAB|^USER_VECT_TAB|^DATA_IN|^SRAM_BASE|^FMC|^FSMC' {
      return 'CMSIS向量表/外存选项，是否生效按条件编译分支判断。'
    }
    default { return '平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。' }
  }
}
$macroDoc = [Text.StringBuilder]::new()
[void]$macroDoc.AppendLine('# 宏逐项参考与源码消费点')
[void]$macroDoc.AppendLine()
[void]$macroDoc.AppendLine('由 [生成脚本](tools/update_technical_reference.ps1) 从 Core 源码生成。原理见 [技术总文档](TECHNICAL_ARCHITECTURE.md)，源码摘录见 [实现说明](IMPLEMENTATION_GUIDE.md)。')
[void]$macroDoc.AppendLine()
[void]$macroDoc.AppendLine('范围包含 Core 内业务宏、私有宏、包含保护和平台配置。Drivers/Middlewares 内部宏使用厂商文档。保留条件编译每个定义位置：值是源表达式，不是编译器覆盖后的生效值。已注释的伪定义不计入；Core引用扫描只作为导航，第三方实际消费以预处理和源代码为准。')
$count=0
foreach ($path in ($cache.Keys | Sort-Object)) {
  $source=$cache[$path]; $masked=Get-MaskedSource $source
  $definitions=[regex]::Matches($masked,'(?m)^[ \t]*#[ \t]*define[ \t]+([A-Za-z_]\w*)')
  if (!$definitions.Count) {continue}
  [void]$macroDoc.AppendLine($newline+"## $path")
  foreach ($definition in $definitions) {
    $name=$definition.Groups[1].Value
    $line=1+([regex]::Matches($source.Substring(0,$definition.Index),'\n')).Count
    $lines=$source.Substring($definition.Index) -split '\r?\n'
    $raw=$lines[0].TrimEnd(); $index=0
    while ($raw.TrimEnd().EndsWith('\') -and $index+1 -lt $lines.Count) {
      ++$index; $raw+=$newline+$lines[$index].TrimEnd()
    }
    if ($raw.Contains('/*') -and !$raw.Contains('*/')) {
      while ($index+1 -lt $lines.Count -and !$raw.Contains('*/')) {
        ++$index; $raw+=$newline+$lines[$index].TrimEnd()
      }
    }
    [void]$macroDoc.AppendLine($newline+"### $name（第 $line 行定义）"+$newline)
    [void]$macroDoc.AppendLine($fence+'c')
    [void]$macroDoc.AppendLine($raw)
    [void]$macroDoc.AppendLine($fence+$newline)
    [void]$macroDoc.AppendLine((Get-Mechanism $name))
    $uses=@()
    foreach ($consumer in ($cache.Keys | Sort-Object)) {
      $consumerLines=$cache[$consumer] -split '\r?\n'
      for($i=0;$i -lt $consumerLines.Count;++$i) {
        if ($consumerLines[$i] -match ('\b'+[regex]::Escape($name)+'\b') -and
            $consumerLines[$i] -notmatch '^\s*#\s*(define|ifndef)' -and
            $consumerLines[$i] -notmatch '^\s*(/\*|\*|//|#endif)') {
          $uses+="[$consumer](../$consumer) 第 $($i+1) 行"; break
        }
      }
    }
    [void]$macroDoc.AppendLine($newline+"源文件：[$path](../$path)。"+$newline)
    if($uses.Count) {[void]$macroDoc.AppendLine('Core引用：'+($uses -join '；')+'。')}
    else {[void]$macroDoc.AppendLine('Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。')}
    ++$count
  }
}
[void]$macroDoc.AppendLine($newline+"共覆盖 $count 个未注释的宏定义位置。")
[IO.File]::WriteAllText((Join-Path $docRoot 'MACRO_REFERENCE.md'),$macroDoc.ToString(),$utf8)

$snippets=@(
 @('Core/Src/bsp/bmi088/bmi088.c','Bmi088_Init','SPI模式切换、复位、ID与可写位回读。'),
 @('Core/Src/bsp/bmi088/bmi088.c','Bmi088_FinishRead','dummy偏移、补码、量程与温度解析。'),
 @('Core/Src/bsp/ist8310/ist8310.c','Ist8310_PollInit','分步复位与配置回读，不暂停融合100ms。'),
 @('Core/Src/bsp/ist8310/ist8310.c','Ist8310_Read','STAT1新数据/覆盖和μT转换。'),
 @('Core/Src/algorithm/ahrs/ahrs.c','Ahrs_Init','保持真实倾角，建立零航向与磁参考。'),
 @('Core/Src/algorithm/ahrs/ahrs.c','Ahrs_Update','实际dt、重力反馈、磁门控和有界航向恢复。'),
 @('Core/Src/app/ins/ins.c','Ins_Calibrate','Welford连续静止窗口与零偏。'),
 @('Core/Src/app/ins/ins.c','Ins_Process','安装/标定转换、融合、角度回绕和快照发布。'),
 @('Core/Src/app/ins/ins.c','Ins_GetSnapshot','短临界区复制并重新检查HAL年龄。'),
 @('Core/Src/task/task_ins/task_ins_runtime.c','InsTask_ReadDma','任务通知有界等待；DMA失败阶段和Abort前SPI错误留证，传感器通知不能冒充完成。'),
 @('Core/Src/task/task_ins/task_ins_runtime.c','InsTask_ImuFault','先记录历史失败出口/时刻/次数，再Abort失效；恢复不清历史原因。'),
 @('Core/Src/task/task_ins/task_ins_runtime.c','InsTask_Log','MotionCal Raw帧与状态日志互斥；普通姿态追加采集故障记录，磁长格式不叠加，DMA非阻塞提交。'),
 @('Core/Src/task/task_ins/task_ins_runtime.c','InsTask_HardwareReady','BMI硬件先决条件；磁采集启用时才检查I2C3/IST EXTI。'),
 @('Core/Src/task/task_ins/task_ins_runtime.c','InsTask_ServiceMag','磁总门关闭直接返回；开启时执行分步恢复和新鲜数据采集。'),
 @('Core/Src/task/task_ins/task_ins_runtime.c','InsTask_RuntimeRunCycle','最新DRDY采集、超时恢复；磁服务受INS_MAG_ENABLE控制。'),
 @('Core/Src/bsp/dbus/dbus.c','Dbus_DecodeFrame','DBUS位解包、大端/小端区别和范围校验。'),
 @('Core/Src/bsp/dbus/dbus.c','Dbus_CopyCompleted','DMA块复制到邮箱；环形数组保留一个空槽。'),
 @('Core/Src/bsp/dbus/dbus.c','Dbus_Process','无观察者的接收适配；复用同一排空、代数检查和累计流程。'),
 @('Core/Src/bsp/dbus/dbus.c','Dbus_ProcessObserved','逐帧解码、epoch核对与累计；合法提交后在临界区外通知只读观察者。'),
 @('Core/Src/task/task_dbus/task_dbus_trace.c','DbusTrace_RecordFrame','接收帧副本、HAL/DWT时间、微秒间隔与独立诊断丢样计数。'),
 @('Core/Src/task/task_dbus/task_dbus_trace.c','DbusTrace_RecordCommand','同周期命令与累计鼠标输入，0.001千分比量化只用于诊断。'),
 @('Core/Src/task/task_dbus/task_dbus_trace.c','DbusTrace_Flush','完整CSV批次经唯一DMA后端提交，忙时保留，接受后出队。'),
 @('Core/Src/bsp/dbus/dbus.c','Dbus_GetSnapshot','同一快照复制并消费累计鼠标位移。'),
 @('Core/Src/task/task_dbus/task_dbus_runtime.c','DbusTask_MapChannel','摇杆死区和raw→千分比。'),
 @('Core/Src/task/task_dbus/task_dbus_runtime.c','DbusTask_RuntimeRunCycle','同一DBUS快照生成四份命令，保留真实时间戳，不二次消费鼠标。'),
 @('Core/Src/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.c','MouseVirtualJoystick_AdvanceAxis','固定锚点和绝对时间控制保持/回中。'),
 @('Core/Src/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.c','MouseVirtualJoystick_Update','去重、时序校验、分轴累计和输出。'),
 @('Core/Src/algorithm/pid/pid.c','Pid_Update','候选积分、微分首帧、限幅和抗积分饱和。'),
 @('Core/Src/algorithm/pid/pid.c','Pid_ConfigureIntegral','默认关闭的变速/梯形积分配置；Reset保留选项。'),
 @('Core/Src/app/chassis/chassis_control.c','ChassisControl_Profile','源普通底盘的直线和平滑对数曲线。'),
 @('Core/Src/app/chassis/chassis_control.c','ChassisControl_Update','三轴输入、四轮混控和独立PID；不访问HAL。'),
 @('Core/Src/task/task_chassis/task_chassis_command.c','ChassisCommand_Submit','Ctrl按下事件、通信代数与一致值副本。'),
 @('Core/Src/task/task_chassis/task_chassis_command.c','ChassisCommand_GetSnapshot','原始HAL时间戳超时门，不消费鼠标。'),
 @('Core/Src/task/task_chassis/task_chassis_runtime.c','ChassisTask_RuntimeRunCycle','唯一底盘任务消费事件、快照、安全门和聚合发送。'),
 @('Core/Src/bsp/m3508_c620/m3508_c620.c','C620_M3508_HandleRxMessage','CAN1 C620标准反馈校验和大端解析。'),
 @('Core/Src/bsp/m3508_c620/m3508_c620.c','C620_M3508_SendAll','首帧/新鲜度/BOFF复核，临界区构建到HAL提交。'),
 @('Core/Src/algorithm/ramp/ramp.c','Ramp_Update','rate×dt_s限制单周期变化。'),
 @('Core/Src/algorithm/filter/low_pass_filter.c','LowPassFilter_Update','速度反馈低通。'),
 @('Core/Src/algorithm/gravity_compensation/gravity_compensation.c','GravityCompensation_Update','int64角差、周期取模和余弦补偿。'),
 @('Core/Src/app/gimbal/gimbal_control.c','GimbalControl_Reanchor','反馈切换边沿捕获当前角，允许机械范围外；清积分、初始化速度滤波并保留电流Ramp。'),
 @('Core/Src/app/gimbal/gimbal_control.c','GimbalControl_UpdateFeedback','只过滤本次人工增量及对应前馈；分离惯性反馈和机械补偿，不裁剪自动纠偏。'),
 @('Core/Src/app/gimbal/gimbal_control.c','GimbalControl_Update','机械反馈适配入口，转成分离反馈后调用同一正式控制器。'),
 @('Core/Src/app/gimbal/gimbal_axis.c','GimbalAxis_CheckStabilization','自稳配置合法性，方向、资格/新鲜度时间及Pitch迟滞范围。'),
 @('Core/Src/app/gimbal/gimbal_axis.c','GimbalAxis_ObserveImu','一次INS快照、重置/新鲜度/连续更新资格和奇异区迟滞，ZYX角速度投影及等效count/rpm换算。'),
 @('Core/Src/app/gimbal/gimbal_axis.c','GimbalAxis_Run','各一次CAN/INS快照、反馈切换/降级、目标保持、输出许可和CAN提交。'),
 @('Core/Src/bsp/gm6020/gm6020.c','Gm6020_UpdateContinuousAngle','首帧中心对齐；后续反馈回绕展开。'),
 @('Core/Src/bsp/gm6020/gm6020.c','Gm6020_SendLocked','完整聚合帧保留另一轴槽位；不保证两轴计算同步。'),
 @('Core/Src/bsp/gm6020/test_gm6020_calibration.c','Gm6020_TestCalibration_Run','只读标定持续零电流；时间、年龄和反馈来自本周期唯一快照，不运行正式闭环。'),
 @('Core/Src/bsp/c610_m2006/c610_m2006.c','C610_M2006_HandleRxMessage','C610协议解码与连续角度。'),
 @('Core/Src/bsp/c610_m2006/c610_m2006.c','C610_M2006_SendAll','发送前安全门、槽位大端和PRIMASK临界区。'),
 @('Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c','C610_M2006_AngleStep_RunCycle','只读手动测量：首帧基准、连续角度增量、零电流和参数隔离。'),
 @('Core/Src/task/task_feed_motor/task_feed_motor_control.c','FeedMotorControl_StartStep','连续角度目标加法边界检查；失败清零并停止。'),
 @('Core/Src/task/task_feed_motor/task_feed_motor_control.c','FeedMotorControl_TargetReached','有符号目标比较与无符号等长位移兜底。'),
 @('Core/Src/task/task_feed_motor/task_feed_motor_control.c','FeedMotorControl_Update','短按一步、长按步间零电流停顿、双Tick计时及电流方向。'),
 @('Core/Src/task/task_feed_motor/task_feed_motor_runtime.c','FeedMotor_RuntimeRunCycle','实际dt、双PWM、许可和M2006输出。'),
 @('Core/Src/bsp/snail_2305/snail_2305.c','Snail2305_ConfigValid','TIM1时钟、PSC/ARR和脉宽合法性。'),
 @('Core/Src/bsp/snail_2305/snail_2305.c','Snail2305_Process','浮点Ramp转整数CCR；不是实际转速。'),
 @('Core/Src/app/log/log.c','Log_TryPrintf','原子所有权、上下文、格式化长度和DMA失败清理。')
)
$impl=[Text.StringBuilder]::new()
[void]$impl.AppendLine('# 关键实现代码与调用说明')
[void]$impl.AppendLine()
[void]$impl.AppendLine('按INS采集/融合→遥控输入→算法/底盘→云台→供弹→日志排列，以下为当前源码原样摘录，不是独立编译的另一套算法。初始化、类型和辅助函数沿源文件链接查阅。重复函数名取最后定义，避开未启用HAL的桩函数。')
[void]$impl.AppendLine()
[void]$impl.AppendLine('从工程根运行 powershell -NoProfile -File doc/tools/update_technical_reference.ps1 可重建本文件和宏索引。原理推导和实测状态仍需同步 [技术总文档](TECHNICAL_ARCHITECTURE.md)。')
foreach($item in $snippets) {
  $path,$name,$purpose=$item; $source=$cache[$path]; $mask=Get-MaskedSource $source
  # 只匹配带返回类型、参数列表并紧跟函数体的定义；不能把调用点或函数指针声明当作定义。
  # 项目状态枚举也以 TypeDef 结尾，不能只支持内置整数返回值。
  $signature = '(?m)^[ \t]*(?:(?:static|inline|__weak)\s+)*(?:void|bool|float|double|char|unsigned|signed|uint8_t|uint16_t|uint32_t|uint64_t|int8_t|int16_t|int32_t|int64_t|size_t|[A-Za-z_]\w*TypeDef|osStatus_t|[A-Za-z_]\w*\s*\*)\s+' + [regex]::Escape($name) + '\s*\([^;{}]*\)\s*\{'
  $foundList=[regex]::Matches($mask,$signature)
  if(!$foundList.Count) {throw "找不到函数定义：$name"}
  $found=$foundList[$foundList.Count-1]; $start=$found.Index
  $open=$mask.IndexOf('{',$start); $level=1; $end=$open+1
  while($end -lt $mask.Length -and $level -gt 0) {
    if($mask[$end] -eq '{') {++$level}
    if($mask[$end] -eq '}') {--$level}
    ++$end
  }
  if($level -ne 0) {throw "大括号不匹配：$name"}
  $line=1+([regex]::Matches($source.Substring(0,$start),'\n')).Count
  [void]$impl.AppendLine($newline+"## $name"+$newline)
  [void]$impl.AppendLine($purpose+$newline)
  [void]$impl.AppendLine("源文件：[$path](../$path)，定义始于第 $line 行。"+$newline)
  [void]$impl.AppendLine($fence+'c')
  [void]$impl.AppendLine($source.Substring($start,$end-$start).TrimEnd())
  [void]$impl.AppendLine($fence)
}
[IO.File]::WriteAllText((Join-Path $docRoot 'IMPLEMENTATION_GUIDE.md'),$impl.ToString(),$utf8)
Write-Output "宏定义位置：$count；关键函数：$($snippets.Count)。"


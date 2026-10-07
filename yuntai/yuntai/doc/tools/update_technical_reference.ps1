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
    'POSITION_KP' { return '位置P：位置误差count乘增益得到rpm（供弹得到电流raw）；见总文档§5/6/7。' }
    'POSITION_KI' { return '位置积分：Ki×误差×dt_s；目标速度饱和且误差向外时恢复上一积分。' }
    'POSITION_KD' { return '位置阻尼：Kd×(命令rpm−滤波rpm)×8192/60。' }
    'POSITION_INTEGRAL' { return '外环积分限制在±该rpm；不是总目标速度上限。' }
    'SPEED_KP' { return '速度P：Kp×(目标rpm−滤波rpm)，输出raw。' }
    'SPEED_KI' { return '速度I：Ki×速度误差×dt_s；候选积分限幅和条件保存。' }
    'SPEED_KD' { return '速度D：Kd×(本误差−上一误差)/dt_s；首帧微分为零。' }
    'SPEED_INTEGRAL' { return '速度PID积分历史限幅，不能替代补偿合成后的总电流限幅。' }
    'FILTER_ALPHA' { return '一阶低通y←y+alpha×(x−y)；权重越小响应越滞后。' }
    'SLEW|RAMP_TIME' { return 'Ramp变化最多rate×dt_s；PWM速率=(活动−停止)×1000/RAMP_MS。' }
    'VELOCITY_FEEDFORWARD' { return '位置外环输出叠加Kff×有效命令rpm；边界向外命令先归零。' }
    'GRAVITY.*BIAS' { return '余弦补偿常值B，模型sign×(B+A×cos(theta))。' }
    'GRAVITY.*AMPLITUDE' { return '余弦补偿幅值A，要求B+A≤补偿模型电流上限。' }
    'GRAVITY.*TWO_PI' { return '编码器角差×2π/8192转换为余弦输入rad。' }
    'GRAVITY.*ENABLE' { return '补偿开关，关闭返回0；禁用模型也校验参数。' }
    'GRAVITY.*COUNTS_PER_REV' { return '补偿角差int64求差后按8192取余，不改写控制目标。' }
    'GRAVITY.*CURRENT_SIGN' { return '补偿在控制器坐标中的方向；合成后仍乘最终轴电流方向。' }
    'MOUSE.*GAIN' { return '新帧：v←clamp(v+相对位移×浮点增益)，单位‰/count，小数保留。' }
    'MOUSE.*HOLD' { return '本轴最后非零事件后保持H毫秒，零位移和另一轴输入不续期。' }
    'MOUSE.*DECAY' { return '回中v=v0×(D−(age−H))/D；age≥H+D归零。' }
    'MOUSE.*OUTPUT_LIMIT' { return '虚拟鼠标单独限幅±L‰；摇杆合成后再限幅±1000‰。' }
    'SIGN' { return '方向映射±1；输入、反馈和电流独立。C615只记录方向，不用PWM切换正反转。' }
    'STALL_COUNTS' { return '单发停滞窗口：连续角度距最近明显移动位置不超过该count；超过则更新参考位置并清零确认计时。' }
    'STALL_CONFIRM_MS' { return '单发位置停滞连续保持该毫秒数后结束C610上弹并开始发射保持；C615已预旋，不负责卡弹超时保护。' }
    'SINGLE_FIRE_HOLD_MS' { return 'C610上弹停滞确认完成后，保持两路C615活动PWM该毫秒数；计时结束进入等待松键或连发。' }
    'CONTINUOUS_PRESS_MS' { return '鼠标左键持续该毫秒数后锁存连发意图；当前单发仍必须先完成。' }
    'FEED_CURRENT_RAW' { return '单发和连发的固定M2006供弹电流raw；由CURRENT_SIGN映射协议方向。' }
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
    'GIMBAL_LOG_OFFSET' { return '百分之一度=(目标−中心)×36000/8192；ABS只用于显示，不参与PID。' }
    'GIMBAL_LOG_FORMAT|GIMBAL_LOG_ARGS' { return '统一日志格式/同周期快照字段；轴参数多次展开，须无副作用。' }
    'LOG_ENABLED_CATEGORY_MASK|LOG_CATEGORY_ENABLED|LOG_TRY_PRINTF|LOG_SWITCH_VALID' {
      return '分类位掩码、0/1开关校验和条件运算；关闭分类不求值格式参数。'
    }
    '^LOG_' { return '编译期日志开关/图表前缀，仅控制诊断，不控制电机使能。' }
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
 @('Core/Src/bsp/dbus/dbus.c','Dbus_DecodeFrame','DBUS位解包、大端/小端区别和范围校验。'),
 @('Core/Src/bsp/dbus/dbus.c','Dbus_CopyCompleted','DMA块复制到邮箱；环形数组保留一个空槽。'),
 @('Core/Src/bsp/dbus/dbus.c','Dbus_Process','邮箱排空、epoch旧帧拒绝和鼠标位移累计。'),
 @('Core/Src/bsp/dbus/dbus.c','Dbus_GetSnapshot','同一快照复制并消费累计鼠标位移。'),
 @('Core/Src/task/task_dbus/task_dbus_runtime.c','DbusTask_MapChannel','摇杆死区和raw→千分比。'),
 @('Core/Src/task/task_dbus/task_dbus_runtime.c','DbusTask_RuntimeRunCycle','同一DBUS快照生成三份命令，保留真实时间戳。'),
 @('Core/Src/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.c','MouseVirtualJoystick_AdvanceAxis','固定锚点和绝对时间控制保持/回中。'),
 @('Core/Src/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.c','MouseVirtualJoystick_Update','去重、时序校验、分轴累计和输出。'),
 @('Core/Src/algorithm/pid/pid.c','Pid_Update','候选积分、微分首帧、限幅和抗积分饱和。'),
 @('Core/Src/algorithm/ramp/ramp.c','Ramp_Update','rate×dt_s限制单周期变化。'),
 @('Core/Src/algorithm/filter/low_pass_filter.c','LowPassFilter_Update','速度反馈低通。'),
 @('Core/Src/algorithm/gravity_compensation/gravity_compensation.c','GravityCompensation_Update','int64角差、周期取模和余弦补偿。'),
 @('Core/Src/app/gimbal/gimbal_control.c','GimbalControl_Update','目标积分、边界过滤、级联环、补偿和Ramp。'),
 @('Core/Src/app/gimbal/gimbal_axis.c','GimbalAxis_Run','一致快照、安全门、恢复、符号和CAN提交。'),
 @('Core/Src/bsp/gm6020/gm6020.c','Gm6020_UpdateContinuousAngle','首帧中心对齐；后续反馈回绕展开。'),
 @('Core/Src/bsp/gm6020/gm6020.c','Gm6020_SendLocked','完整聚合帧保留另一轴槽位；不保证两轴计算同步。'),
 @('Core/Src/bsp/c610_m2006/c610_m2006.c','C610_M2006_HandleRxMessage','C610协议解码与连续角度。'),
 @('Core/Src/bsp/c610_m2006/c610_m2006.c','C610_M2006_SendAll','发送前安全门、槽位大端和PRIMASK临界区。'),
 @('Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c','C610_M2006_AngleStep_RunCycle','只读手动测量：首帧基准、连续角度增量、零电流和参数隔离。'),
 @('Core/Src/task/task_feed_motor/task_feed_motor_control.c','FeedMotorControl_Update','单发先预旋、上弹期间持续PWM、停滞确认结束上弹、短按/长按和连发状态机。'),
 @('Core/Src/task/task_feed_motor/task_feed_motor_runtime.c','FeedMotor_RuntimeRunCycle','实际dt、双PWM、许可和M2006输出。'),
 @('Core/Src/bsp/snail_2305/snail_2305.c','Snail2305_ConfigValid','TIM1时钟、PSC/ARR和脉宽合法性。'),
 @('Core/Src/bsp/snail_2305/snail_2305.c','Snail2305_Process','浮点Ramp转整数CCR；不是实际转速。'),
 @('Core/Src/app/log/log.c','Log_TryPrintf','原子所有权、上下文、格式化长度和DMA失败清理。')
)
$impl=[Text.StringBuilder]::new()
[void]$impl.AppendLine('# 关键实现代码与调用说明')
[void]$impl.AppendLine()
[void]$impl.AppendLine('按输入→算法→云台→供弹→日志排列，以下为当前源码原样摘录，不是独立编译的另一套算法。初始化、类型和辅助函数沿源文件链接查阅。重复函数名取最后定义，避开未启用HAL的桩函数。')
[void]$impl.AppendLine()
[void]$impl.AppendLine('从工程根运行 powershell -NoProfile -File doc/tools/update_technical_reference.ps1 可重建本文件和宏索引。原理推导和实测状态仍需同步 [技术总文档](TECHNICAL_ARCHITECTURE.md)。')
foreach($item in $snippets) {
  $path,$name,$purpose=$item; $source=$cache[$path]; $mask=Get-MaskedSource $source
  # 只匹配带返回类型、参数列表并紧跟函数体的定义；不能把调用点或函数指针声明当作定义。
  $signature = '(?m)^[ \t]*(?:(?:static|inline|__weak)\s+)*(?:void|bool|float|double|char|unsigned|signed|uint8_t|uint16_t|uint32_t|uint64_t|int8_t|int16_t|int32_t|int64_t|size_t|HAL_StatusTypeDef|osStatus_t|[A-Za-z_]\w*\s*\*)\s+' + [regex]::Escape($name) + '\s*\([^;{}]*\)\s*\{'
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


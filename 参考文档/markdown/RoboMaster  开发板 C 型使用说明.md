# RoboMaster  开发板 C 型使用说明

> Source: `RoboMaster  开发板 C 型使用说明.pdf`; extracted with pypdf 6.10.0. Layout, figures and tables may require the original PDF.

## Page 1

RoboMaster 
Development Board Type C 
开发板 C 型
開発ボード タイプ C
2019.11v1.0
Quick Start Guide
快速入门指南
ユーザーガイド

## Page 2

1
EN 
Disclaimer
Thank you for purchasing the ROBOMASTER
TM
 Development Board Type C 
(hereinafter referred to as “Board Type C”). Read this disclaimer carefully before 
using this product. By using this product, you hereby agree to this disclaimer 
and signify that you have read it carefully. Install and use this product in strict 
accordance with all related documents. Users bear the responsibilities in 
all the consequences caused by using this product. DJI
TM
 will not bear any 
legal responsibilities for any damages due to improper use, installation, or 
modification.
DJI and ROBOMASTER are trademarks of DJI and its affiliated companies. Names 
of products, brands, etc., appearing in this document are trademarks or registered 
trademarks of their respective owner companies. This product and document are 
copyrighted by DJI with all rights reserved. No part of this product or document 
In the Box / 物品清单 / 同梱物リスト
Board Type C
开发板 C 型
ボード タイプ C
2-Pin Cable / 2-Pin 线 / 2 ピンケーブル
4-Pin Cable / 4-Pin 线 / 4 ピンケーブル
SWD Cable / SWD 下载线 / SWD ケーブル 
Power Cable / 电源线 / 電源ケーブル

## Page 3

2
shall be reproduced in any form without the prior written consent or authorization 
of DJI. The final interpretation right of this disclaimer is reserved by DJI. 
This document and all other collateral documents are subject to change at 
the sole discretion of DJI. For up to date product information, visit http://www.
robomaster.com and click on the product page for this product.
Warning
1. Visit the RoboMaster official website and download the RoboMaster Development 
Board Type C User Manual before use. Read and understand the whole manual 
and strictly follow the instructions in the manual when using the Board Type C.
2. Connect the cables correctly by following the instructions in the RoboMaster 
Development Board Type C User Manual. Otherwise, the cables or the Board 
Type C may be seriously damaged.
3. Make sure there are no short-circuits and all the cables are in good condition. 
DO NOT use cables that have been damaged in any way.
4. Make sure to use the product in strict accordance with the specifications listed 
in this document, including those related to voltage and temperature. Failure to 
do so may reduce the product service life or even lead to permanent damage.
5. To avoid physical damage, make sure to assemble the Board Type C correctly.
6. If you detect any flames, smoke, strange smells, or other abnormalities, 
disconnect the Board Type C from the power source immediately.
7. DO NOT open the silicone case. Otherwise, foreign objects may fall inside and 
the performance of the Development Board Type C may be negatively affected.
Introduction
Designed to work with the RoboMaster and other products, the compact 
Development Board Type C uses a high-performance STM32 microcontroller chip 
and supports a wide voltage input. The highly integrable Board Type C boasts 
an expansion interface, communication interface and high precision IMU sensors 
and features an anti-reverse connection and anti-overvoltage protection. The 
Board Type C can provide rich routines and can be widely used in fields such 
as robotics competitions, research and education, and automation equipment.

## Page 4

3
Interface Description
1. Customizable LED
2. 5V Port
3. Reset Button
4. Micro USB Port
5. BOOT Configuration 
Port
6. SWD Port
7. Customizable Button
8. 24 V Power Input Port
9. 24 V Power Output 
Port111212 131314
16
15
123 4 5 6 7
8
9
10
9
9
17
10. Customizable I/O 
Port
11. 3-Pin UART Port
12. CAN2 Port
13. CAN1 Port
14. 4-Pin UART Port
15. PWM Port
16. DBUS Port
17. 18-Pin Digital 
Camera FPC Port

## Page 5

4
Unit: mm
Unit: mm
Mounting the Board Type C
Refer to the dimensions in the figure below when mounting the Board Type C.
The Board Type C can be used with the RoboMaster ESC Center Board 2 for an 
extension port, shown as below.
Note: Screws and copper pillars are not included.
4− 5.00 4− 2.50
60.00
36.00
M2.5-6 Screw (4 pcs)
M2.5-20 Double Pass Copper Pillar (4 pcs)

## Page 6

Usage
The firmware of the Board Type C can be updated via SWD or 
DFU mode. Scan the QR code or go to https://www.robomaster.
com/en-US/products/components/general/development-board-
type-c#downloads to download the RoboMaster Development 
Boards User Manual for further information.
Specifications
Input Voltage 8 - 28 V
Operating Current* 0.01 A @ DC 24 V
Weight 38 g
Dimensions 60 × 41 × 16 mm
Operating Temperature Range 0 - 55° C (0 - 131 °F)
* Tested at a room temperature of 25° C (77 °F) and in a well-ventilated lab 
environment.
5
CHS
免责声明
感谢您购买 RoboMaster
TM
 开发板 C 型（以下简称“开发板”）。在使用之前，
请仔细阅读本声明，一旦使用，即被视为对本声明全部内容的认可和接受。请严
格遵守手册、产品说明和相关的法律法规、政策、准则安装和使用该产品。在使
用产品过程中，用户承诺对自己的行为及因此而产生的所有后果负责。因用户不
当使用、安装、改装造成的任何损失，DJI
TM
将不承担法律责任。
DJI 和 RoboMaster 是深圳市大疆 
TM
 创新科技有限公司及其关联公司的商标。本
文出现的产品名称、品牌等，均为其所属公司的商标。本产品及手册为大疆创新
版权所有。未经许可，不得以任何形式复制翻印。关于免责声明的最终解释权，
归大疆创新所有。
本文档及本产品所有相关的文档最终解释权归大疆创新所有。如有更新，恕不另
行通知。请访问 www.robomaster.com 官方网站以获取最新的产品信息。

## Page 7

6
1. 自定义 LED
2. 5V 接口
3. 复位按键
4. Micro USB 接口
5. BOOT 配置接口
6. SWD 下载接口
7. 自定义按键
8. 24V 电源输入接口
9. 24V 电源输出接口
10. 可配置 I/O 接口
11. UART 接 口（3-pin）
12. CAN2 总线接口
产品使用注意事项
1. 使用前，请前往 RoboMaster 官网下载《RoboMaster 开发板 C 型用户手册》，
仔细阅读里面的注意事项并了解具体使用方法。
2. 请按照说明正确使用线材，以免损坏线材或者开发板。
3. 使用前，请检查线材有无老化、损坏。如存在以上现象，请更换新线材。
4. 请按照说明在规定的工作环境（如电压、温度等参数）使用，否则可能会影响
产品寿命或造成永久性损坏。
5. 请使用正确的方式固定开发板，避免开发板受到物理损坏。
6. 开发板通电后如发现有火花、冒烟、焦糊味或其它异常，请立即关掉电源。
7. 使用时，请不要掀开硅胶外壳，避免由于异物造成开发板短路或性能下降。
简 介
RoboMaster 开发板 C 型采用高性能的 STM32 主控芯片，支持宽电压输入，集
成专用的扩展接口、通信接口以及高精度 IMU 传感器，可配合 RoboMaster 产品
或其它产品使用。开发板具备防反接、防过压等保护功能，结构紧凑、集成度高、
配套例程丰富，可广泛应用在机器人比赛、科研教育、自动化设备等领域。
接口说明
111212 131314
16
15
123 4 5 6 7
8
9
10
9
9

## Page 8

7
13. CAN1 总线接口
14. UART 接口（4-pin）
15. PWM 接口
16. DBUS 接口
17. 数字摄像头 FPC 接口
（18-pin）
尺寸及安装说明
请参考图示尺寸，正确安装开发板。
单位：mm
17
4− 5.00 4− 2.50
60.00
36.00

## Page 9

8
此外，开发板可搭配 RoboMaster 电调中心板 2 实现接口扩展，如下图所示。
使用
开发板支持 SWD 或 DFU 下载固件，请扫描二维码或前往官网 
https://www.robomaster.com/zh-CN/products/components/
general/development-board-type-c#downloads，下载
《RoboMaster 开发板 C 型用户手册》了解详细操作方法。
特征参数
输入电压 8V~28V
待机电流 * 0.01A@DC 24V
重量 38g
尺寸（长 × 宽 x 高） 60×41×16mm
工作温度范围 0~55℃
* 室温 25℃、通风良好的实验环境下测得。
（备注：螺丝及铜柱需自行购买） 单位 : mm
M2.5-6螺丝（4颗）
M2.5-20双通铜柱（4颗）

## Page 10

9
JP
免責条項
この度はROBOMASTER
TM
 開発ボード タイプ C （以下、「ボード タイプ C 」
といいます）をお買い上げいただき誠にありがとうございます。本製品のご使
用前に、この免責事項をよくお読みください。本製品を使用すると、この免責
事項をすべて読み、これに同意したとみなされます。すべての関連文書に従っ
て、本製品をインストールし、ご使用ください。ユーザーは、本製品を使用す
ることによって生じるすべての結果に責任を負います。 DJI
TM
 は、不適切な使
用、取り付け、または変更による損害に対して何ら責任を負いません。
DJIおよびROBOMASTER はDJIおよびその関連会社の商標です。本書に記載さ
れている製品、ブランドなどの名称は、その所有者である各社の商標または登
録商標です。本製品および本書は、不許複製・禁無断転載を原則とするDJIの著
作物のため、DJIから書面による事前承認または許諾を得ることなく、本製品ま
たは文書のいかなる部分も、いかなる方法によっても複製することは固く禁じ
られています。本免責事項の最終解釈権限はDJIが有します。
本書およびその他すべての付属書は、 DJI独自の裁量で変更されることがあり
ます。最新の製品情報については、http://www.robomaster.comにアクセスのう
え、本製品に対応する製品ページをクリックしてご覧ください。
警告
1. ご使用前に、 RoboMaster 公式ウェブサイトにアクセスして『 RoboMaster 
開発ボード タイプ C ユーザーマニュアル』をダウンロードしてください。
ボード タイプ Cを使用する際は、マニュアルをすべて読み、内容を理解し、
マニュアル記載の指示を厳守してください。
2. 『RoboMaster 開発ボード タイプ C ユーザーマニュアル』記載の指示に従っ
てケーブルを正しく接続してください。正しく接続されない場合、ケーブル
またはボード タイプ Cが重大な損傷を受ける可能性があります。
3. 短絡がなく、すべてのケーブルが良好な状態であることを確認してくださ
い。損傷したケーブルは決して使用しないでください。

## Page 11

10
4. 本製品は、本書記載の各仕様（電圧や温度など）を厳守してご使用くださ
い。遵守しない場合には製品寿命が短くなったり、製品に修復不能な損傷が
発生したりするおそれがあります。
5. 物理的な損傷を避けるため、ボード タイプ Cを正しく組み立ててください。
6. 火炎、煙、異臭などの異常を検知した場合は、速やかにボード タイプ Cを電
源から取り外してください。
7. シリコンケースを開けないでください。開けると、異物が内部に侵入し、開
発ボード タイプ Cの動作に悪影響を及ぼす可能性があります。
はじめに
RoboMasterおよび他の製品と連携するように設計されたコンパクトな開発ボー
ド タイプ C は、高性能STM32マイクロコントローラーチップを使用し、幅広
い入力電圧に対応しています。高度に統合可能なボード タイプ Cは、拡張イン
ターフェース、通信インターフェース、高精度 IMUセンサーを搭載し、逆接続
保護機能と過電圧保護機能を備えています。ボード タイプ Cは豊富なルーチン
を提供でき、ロボット大会、研究と教育、自動化機器などの分野で幅広く使用
できます。
インターフェースの説明
1. カスタムLED
2. 5Vポート
3. リセットボタン
4. Micro-USBポート
5. ブート構成ポート
6. SWDポート
7. カスタムボタン
8. 24V電源入力ポート
9. 24V電源出力ポート
111212 131314
16
15
123 4 5 6 7
8
9
10
9
9

## Page 12

11
17
10. カスタム I/Oポート
11. 3ピンUARTポート
12. CAN2ポート
13. CAN1ポート
14. 4ピンUARTポート
15. PWMポート
16. DBUSポート
17. 18ピンデジタルカ
メラFPCポート
単位：mm
4− 5.00 4− 2.50
60.00
36.00
ボード タイプ Cの取り付け
ボード タイプ Cの取り付けの際は、下図の寸法を参照してください。

## Page 13

12
単位：mm
次に示すように、ボード  タイプ C は、拡張ポート用の RoboMaster ESC セン
ターボード2で使用できます。
注：ネジと銅柱は含まれていません。
使用方法
ボード タイプ C のファームウェアは、 SWD またはDFU モー
ドで更新できます。詳細については、 QR コードをスキャ
ンするか、 https://www.robomaster.com/en-US/products/
components/general/development-board-type-c#downloadsに
アクセスして、『RoboMaster 開発ボード ユーザーマニュア
ル』をダウンロードしてご確認ください。
仕様
入力電圧 8～28 V
動作電流* 0.01 A @ DC 24 V 
重量 38 g
サイズ 60×41×16 mm
動作環境温度範囲 0～55℃
* 換気の良い、室温25℃のラボ環境下でテスト済み。
M2.5-6 ω δʢ4ʣ
M2.5-20 μϒϧύεಔப ʢ4ʣ

## Page 14

Printed in China.
中国印制
 and  are trademarks of DJI.
Copyright © 2019 DJI All Rights Reserved.
 和  是大疆创新的商标。
Copyright © 2019 大疆创新 版权所有
WWW.ROBOMASTER.COM
YC.BZ.SS001152.02

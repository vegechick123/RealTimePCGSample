# RealTimePCGSample

## **使用说明**

### 一、  启动插件

用户启动已经安装的Unreal Engine插件，

 

1. 在左上角的Edit中选中Plugin

 

![电脑萤幕的截图  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image002.gif)

1. 在Installed中找到RealTimePCGFoliage，将对应的Enabled勾选

![图形用户界面, 文本, 应用程序  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image004.gif)

 



 

 

### 二、  关卡初始设置

1. 搜索PCGFoliageManager,并将其拖入到关卡内，将其位置设为（0，0，0）

![电脑萤幕的截图  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image006.gif)

 

2. 创建地形，将Mode切换为LandScape，并在左边的面板上勾选Enable Edit Layers后点击Create创建地形。

![图形用户界面  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image008.gif)

3. 参数设置，创建完毕后将Mode切换回Select，选中之前创建的PCGFoliageManager,将其Landscape参数设置为刚刚创建的地形，勾选AutoCaptureLandscape，并将TextureSize和RenderTargetSize分别设置为（128，128）和（512，512）。

![电脑屏幕的截图  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image010.gif)



 

### 三、  物种的设置

1. 实例创建，在文件系统中右键，在弹出的菜单里中找到RealTimeProcedualFoliage中的Species，点击以创建出一个物种实例

![图形用户界面, 网站  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image012.gif)

2. 参数配置，点击刚刚创建的实例，并将半径、密度、模型、密度分布等参数设置好。

![图形用户界面  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image014.gif)



 

### 四、  群落的设置

1. 实例创建，在文件系统中右键，在弹出的菜单里中找到RealTimeProcedualFoliage中的Biome，点击以创建出一个群落实例。

![图片包含 图形用户界面  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image016.gif)

2. 参数配置，将对应的物种拖入到刚才创建的群落中

![图形用户界面, 应用程序  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image018.gif)

3. 将需要用到的群落拖入到PCGFoliageManager的Biomes中。

![图形用户界面, 应用程序  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image020.gif)



 

 

### 五、  植被生成绘制

1. 将Mode切换为RealTimePCGFoliageEdMode。

![图形用户界面, 应用程序  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image022.gif)

2. 选中想要绘制的群落

![图形用户界面  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image024.gif)



3. 在关卡窗口中按住左键拖动，在想要生成植被的位置进行绘制。

![图形用户界面, 应用程序  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image026.gif)

4. 在关卡窗口上方选中PaintSpecies，并在左边的面板下方选中想要擦除的物种，可以以绘制的方式清除指定位置的指定物种。

![图形用户界面, 网站  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image028.gif)

### 六、  水体交互

1. 本插件使用Water Plugin插件来作为水体交互的对象，所以需要先启用Water Plugin，在Edit->Plugin中找到Water，并勾选Enabled。

![图形用户界面, 文本  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image030.gif)

2. 将需要的Water Body拖入到关卡中，这里用WaterBodyLake作为示例。

![电脑游戏的截图  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image032.gif)

3. 将PCGFoliageManager中的TextureCollectComponent中的WaterBrushManager参数设置为新创建的WaterBrushManager。



![图形用户界面, 应用程序  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image034.gif)

4. 调整水体范围。

![图片包含 室内, 蛋糕, 桌子, 亮  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image036.gif)

5. 点击PCGFoliageManager上的Rebuild参数，重新生成植被。

![图形用户界面  描述已自动生成](file:///C:/Users/VEGECH~1/AppData/Local/Temp/msohtmlclip1/01/clip_image038.gif)

 

# RealTimePCGSample
### Unreal4中基于GPU的实时植被生成插件
[知乎文章链接](https://zhuanlan.zhihu.com/p/469317645)
## **使用说明**

### 一、  启动插件

用户启动已经安装的Unreal Engine插件，

 

1. 在左上角的Edit中选中Plugin

 

![image](https://user-images.githubusercontent.com/47053926/183250219-52d94bc6-b22a-4594-921c-a74811a0056b.png)


1. 在Installed中找到RealTimePCGFoliage，将对应的Enabled勾选

![image](https://user-images.githubusercontent.com/47053926/183250228-ca5b315e-0cf5-4a95-b011-923959efb951.png)

 



 

 

### 二、  关卡初始设置

1. 搜索PCGFoliageManager,并将其拖入到关卡内，将其位置设为（0，0，0）

![image](https://user-images.githubusercontent.com/47053926/183250245-28f4944a-1a6f-4536-817f-3378b3a612b8.png)

 

2. 创建地形，将Mode切换为LandScape，并在左边的面板上勾选Enable Edit Layers后点击Create创建地形。

![image](https://user-images.githubusercontent.com/47053926/183250253-d7109390-8a7a-4fab-836d-575b139ca65a.png)

3. 参数设置，创建完毕后将Mode切换回Select，选中之前创建的PCGFoliageManager,将其Landscape参数设置为刚刚创建的地形，勾选AutoCaptureLandscape，并将TextureSize和RenderTargetSize分别设置为（128，128）和（512，512）。

![image](https://user-images.githubusercontent.com/47053926/183250257-6c8dabf4-831e-4690-bd28-5139e42935fe.png)



 

### 三、  物种的设置

1. 实例创建，在文件系统中右键，在弹出的菜单里中找到RealTimeProcedualFoliage中的Species，点击以创建出一个物种实例

![image](https://user-images.githubusercontent.com/47053926/183250281-8e09bbfd-c55b-4201-bbcb-af1fd80209d3.png)

2. 参数配置，点击刚刚创建的实例，并将半径、密度、模型、密度分布等参数设置好。

![image](https://user-images.githubusercontent.com/47053926/183250284-2c718115-0c4a-4633-8312-0d7cf499cc4f.png)



 

### 四、  群落的设置

1. 实例创建，在文件系统中右键，在弹出的菜单里中找到RealTimeProcedualFoliage中的Biome，点击以创建出一个群落实例。

![image](https://user-images.githubusercontent.com/47053926/183250289-8476cdaa-4a99-4000-8414-26975a251dac.png)

2. 参数配置，将对应的物种拖入到刚才创建的群落中

![image](https://user-images.githubusercontent.com/47053926/183250296-515ff7c6-44a2-45fc-b3fe-41c32769bdc5.png)

3. 将需要用到的群落拖入到PCGFoliageManager的Biomes中。

![image](https://user-images.githubusercontent.com/47053926/183250298-d820f545-6c5f-4c84-90c1-ba99b403987c.png)



 

 

### 五、  植被生成绘制

1. 将Mode切换为RealTimePCGFoliageEdMode。

![image](https://user-images.githubusercontent.com/47053926/183250303-939e5397-611a-4121-a2bb-a8421d474659.png)

2. 选中想要绘制的群落

![image](https://user-images.githubusercontent.com/47053926/183250309-05817459-d8e2-4e3e-a573-01b54603082b.png)



3. 在关卡窗口中按住左键拖动，在想要生成植被的位置进行绘制。

![image](https://user-images.githubusercontent.com/47053926/183250314-ea6ff8a1-6efb-4d66-b9f1-40052718fb31.png)

4. 在关卡窗口上方选中PaintSpecies，并在左边的面板下方选中想要擦除的物种，可以以绘制的方式清除指定位置的指定物种。

![image](https://user-images.githubusercontent.com/47053926/183250327-75607bbd-bdd6-4655-803f-e27ee1533c41.png)

### 六、  水体交互

1. 本插件使用Water Plugin插件来作为水体交互的对象，所以需要先启用Water Plugin，在Edit->Plugin中找到Water，并确保Enabled已经勾选。

![image](https://user-images.githubusercontent.com/47053926/183250330-06db9ef4-cfcb-4282-b793-2bb40f185ebe.png)

2. 将需要的Water Body拖入到关卡中，这里用WaterBodyLake作为示例。

![image](https://user-images.githubusercontent.com/47053926/183250332-ea3b7485-6426-44f5-8679-b9f140955fcc.png)

3. 将PCGFoliageManager中的TextureCollectComponent中的WaterBrushManager参数设置为新创建的WaterBrushManager。



![image](https://user-images.githubusercontent.com/47053926/183250336-0346f6b9-048b-4f6e-a882-60d5b2e0c370.png)

4. 调整水体范围。

![image](https://user-images.githubusercontent.com/47053926/183250339-240e9495-9fdc-4165-906c-d877c1972218.png)

5. 点击PCGFoliageManager上的Rebuild参数，重新生成植被。

![image](https://user-images.githubusercontent.com/47053926/183250343-ab2aacc8-68d9-4425-a577-4840c08c9fe1.png)

 

# ProtobufToUEStruct
此插件能将Protobuf的结构转换为了UE的结构体和枚举，支持消息嵌套和枚举嵌套，转换完成的蓝图位于 /Game/Code_Struct/Proto 中将会按照命名空间进行分类

## 使用方法

首先要说明的是，因为是自己用的所以会有点糙，但能用，所以请不要吐槽使用方法麻烦的问题；

1.创建一个json文件，结构类似如下：

{
  "TargetArray":[
    "BaseModeNameSpace.C_BaseAsk"
  ]
}

其中BaseModeNameSpace.C_BaseAsk是你的Proto文件中想要转换的消息，必须使用 此消息的命名空间.消息名称的格式书写

2.点击编辑器窗口的这个按钮
![image](https://github.com/QSWWLTN/ProtobufToUEStruct/assets/52273933/71c29676-f9d9-42af-8b80-4423c1768817)

将会出现选择Proto文件的窗口
![image](https://github.com/QSWWLTN/ProtobufToUEStruct/assets/52273933/e7f71b38-bedc-4c3b-ac45-40523d360e8f)

选择完成后会弹出选择配置文件窗口
![image](https://github.com/QSWWLTN/ProtobufToUEStruct/assets/52273933/17740d77-4bd3-44c5-b8c6-c51c5edeff69)

选择完成后将自动的转换结构，并将原来的蓝图结构体删除。

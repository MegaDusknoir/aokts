关于 Trigger Studio (触发工作室)  
--------------------  
  
触发工作室 (Trigger Studio) 是开源的《帝国时代II》和《星球大战：银河战场》的场景设计辅助工具，用于补充游戏内编辑器所未提供的各种功能。  
  
触发工作室也称AOKTS (帝国时代2触发工作室)，SWGBTS (星球大战：银河战场触发工作室)，或直接简称TS。  
  
官方发布页：[AoK Heaven](http://aok.heavengames.com/blacksmith/showfile.php?fileid=12103&amp;f=&amp;st=40&amp;ci=)  
翔鹰发布页：[翔鹰帝国论坛](https://www.hawkaoe.net/bbs/thread-144770-1-1.html)  
  
触发工作室可以：  
  
* 解压缩场景文件，以便用十六进制编辑器进行编辑。  
* 在不同版本的游戏之间转换场景。  
* 变更单位类型、旋转、位置、海拔、驻扎，ID 等等，来实现各种场景效果，例如控制其他玩家的单位，无敌单位，隐形单位，放置火焰，解锁 Beta 版单位等等。  
* 进行负进贡和负损害。  
* 导出/查看和编辑场景中包含的 AI 文件。  
* 复制，剪切，粘贴和重新排列触发组。  
* 复制，剪切，粘贴，移动和复制地图单位，地形，海拔和触发。  
* 小地图显示所有单位，地形，海拔和触发，且可以缩放并另存为位图文件。  
* 自定义单位列表分组，而非原版的 4 个单位分组。  
* 从头开始创建场景。  
* 诸如此类...  
  
如何解压缩或压缩原始数据  
  
两种方式：  
  
* 打开 TS，然后在“工具”下单击相应的菜单项，选择源文件和目标文件。  
* 从 Windows“运行”对话框运行 TS，使用 -c 进行压缩，使用 -u 进行解压缩。之后键入源文件的路径，接着键入目标文件的路径。  
  
如何将十六进制编辑的数据保存到场景中  
  
* 打开要在 TS 中进行十六进制编辑的场景。  
* 保持 TS 处于打开状态，并在十六进制编辑器中打开 scendata.tmp。  
* 对 scendata.tmp 进行所需的更改，保存并关闭十六进制编辑器。  
* 切换到 TS，然后从“场景”菜单中选择“将 scendata.tmp 保存到SCX ...”，指定要保存的文件。您对 scendata.tmp 的所有更改将保存到场景中，TS 中所做的任何更改都不会保存。  
  
代码  
----  
  
它是使用基于标准Win32 API的C++编写的。  
  
在 VS2005 或 VS2008 上编译  
---------------------------  
  
Make sure target is set to Release, not Debug.  
  
在较新版本的 Visual Studio 上编译  
------------------------------------------  
  
Make sure target is set to Release, not Debug.  
  
Delete ccpp11compat.h  
Change copy_if to std::copy_if  
  
std::bad_alloc error:  
    Remove the string parameter.  
    http://stackoverflow.com/questions/11133438/why-the-bad-allocconst-char-was-made-private-in-visual-c-2012  
  
贡献  
------  
  
### 原始版本， David Tombs (DiGiT) ###  
DIGIT感谢了以下人员：Berserker Jerker，zyxomma，geebert，scenario_t_c和iberico！  
再次感谢Geebert，其在mullikine发布新版本时又进行了出色的反馈！  
  
### 1.0.2 版，danielpereira ###  
* 效果改变对象名称、巡逻，条件拥有对象少于支持区域选择。  
* 新效果会显示正确的名称，并且可以正确地查看和编辑。  
* 反向条件有效（只需将条件中的“反向”的值从-1更改为-256）  
  
### 1.1 版，JustTesting1234 ###  
* 修复了触发名称在复制粘贴后多余字符的问题。  
* 修复了禁用列表（仅限 HD）。  
* 添加了超大（480x480）地图尺寸支持（仅限 HD）。  
* 添加了“锁定组队”复选框 (仅限 HD).  
  
### 1.2 版，E/X mantis ###  
感谢以下人员！Gallas_HU (voobly), [MMCrew]jizzy, Rewaider 和 Lord Basse.  
  
### 1.2 版以后，mullikine ###  
感谢 mullikine 的 github 项目和杰出贡献，没有这个开源项目就没有这个版本的诞生。  
  
待办事项和紧急BUG  
----------------------  
见 [todo.md](todo.md)  
  
开源协议  
-------  
**GNU GPLv2** 或更高版本；参见 [legal/gpl-2.0.txt](legal/gpl-2.0.txt) 或 [legal/gpl-3.0.txt](legal/gpl-3.0.txt).

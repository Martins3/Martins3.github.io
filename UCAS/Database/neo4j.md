一、 （熟悉python、neo4j、cypher、知识图谱相关知识，搭建实战开发环境，跑通示例程序。
	1. anaconda安装
	2. neo4j安装
二、 （根据实际业务需求建立数据模型，将原始数据进行清洗后导入图数据库。建立数据模型，可以查询以下问题：
	1. 电阻值大于x，尺寸小于y的电阻有哪些？
	2. 温度范围在-40~120℃之间的国产振荡器有哪些？
	3. 对国产和进口器件进行分类；
	4. 质量等级为gjb级，质量小于500g的元器件有哪些？
三、 根据业务需求设计并实现智能搜索/智能问答系统，解决所有业务需求。

## 新数据库技术实战练习（基于知识图谱的智能问答系统） 

对半结构化数据进行清洗和处理，在图数据库中进行存储，通过cypher语句对图数据库进行查找，并实现基于图数据库的典型应用。 

#### 实战目的： 
熟悉Python编程——Python是现阶段热度最高的脚本语言，上手快、语法简单，适用于科学计算、原型系统等开发。 
熟悉图数据库Neo4J的相关使用，Neo4J是现阶段排名第一的图数据库，熟悉通过cypher语言对数据库进行操作。 
熟悉典型的查找和智能问答系统开发，现阶段查找和问答均以知识图谱进行提高，知识图谱可以用图数据库进行数据存储。本实战练习旨在熟悉相关的技术以及开发流程，完成一套原型系统。 

实战练习涉及的关键技术： 
1. Python编程 
2. 图数据库的相关操作（使用cypher语言进行创建、查询） 
3. 基于图数据库的查询、智能问答系统（基于模板的智能问答） 

半结构化数据： 
1. 通过爬虫程序爬取的某网站元器件数据（3W条）； 
2. 数据格式为csv格式，采用属性名称+属性值的存储形式； 

任务目标： 

1. 对原始数据进行建模，以实体-关系-实体，实体-属性-属性值两种形式对源数据进行拆分，建立存储模型，存入数据库； 
2. 对原始数据中的字符数据转换为数值数据，便于后续数据库查询； 
3. 在图数据库中进行查询，包括实体查询、关系查询、实体-关系查询、实体-实体查询； 

可以查找以下内容： 

1. 额定功耗为0.15W的北京生产的氧化膜电阻器有哪些 
2. 温度范围可包含-50～150℃的陕西西安生产的射频同轴电连接器有哪些。 
3. 基于数据库实现智能问答的原型系统（选做）。 

相关工具： 
Anaconda 
Python3.6 
Neo4J数据库 
Py2neo工具包 

参考工程：https://github.com/liuhuanyong/QAonMilitaryKG 


## neo4j 使用记录
ftp://neo4j.55555.io/neo4j-desktop/1.2.1/

A graph database can store any kind of data using a few simple concepts:
1. Nodes - graph data records
1. Relationships - connect nodes
1. Properties - named data values

Label 
Editor

find : match where return 似乎

## 代码分析
    def read_nodes_rel(self):

1. 定义关系和实体的dic
2. read_node 函数 ? read_nodes_rel　就是添加内容而已
3. create_node 向Node 中间添加 (莫名奇妙的return) 
4. 一个create_node -> fault 为什么没有处理 ?
5. fault 似乎是关系 ? 


1. clean data
2. cypher query


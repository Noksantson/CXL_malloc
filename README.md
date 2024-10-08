# CXL_malloc

我实现的功能就是通过buddy分配器将256MB大小的area以最小2MB的粒度分配给用户进程

详细算法参考了这个作者的算法
https://coolshell.cn/articles/10427.html

通过使用一个二叉树来管理area的内存，所以在area结构体中又添加了一项维护信息，这个需要拿到area的主机进行初始化才可以。

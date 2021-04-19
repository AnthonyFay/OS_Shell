# OS2021_Project1.Shell



#### yeeshell.c

main function and function definition

#### yeeshell.h

function declaration and macro definition



### 1. Deployment

Pull from my shuishan git.

```shell
git clone root@gitea.shuishan.net.cn:10195501441/OS2021_Project1.Shell.git
```

In the **MINIX3** environment, use clang to compile the program.

```shell
clang yeeshell.c -o yeeshell
```

Run yeeshell.

```shell
./yeeshell
```



### 2. Test Case

​	(1) `cd /your/path`

<img src="http://gitea.shuishan.net.cn/10195501441/OS2021_Project1.Shell/raw/branch/master/lab_report/test_case_img/cd.png" alt="cd" style="zoom: 76%;" />

​	(2) `ls -a -l`

<img src="http://gitea.shuishan.net.cn/10195501441/OS2021_Project1.Shell/raw/branch/master/lab_report/test_case_img/ls.png" alt="ls" style="zoom:76%;" />

​	(3) `ls -a -l > result.txt`

<img src="http://gitea.shuishan.net.cn/10195501441/OS2021_Project1.Shell/raw/branch/master/lab_report/test_case_img/redirect_out.png" alt="redirect_out" style="zoom:76%;" />

​	(4) `vi result.txt`

<img src="http://gitea.shuishan.net.cn/10195501441/OS2021_Project1.Shell/raw/branch/master/lab_report/test_case_img/vi.png" alt="vi" style="zoom:76%;" />

​	(5) `grep a < result.txt`

<img src="http://gitea.shuishan.net.cn/10195501441/OS2021_Project1.Shell/raw/branch/master/lab_report/test_case_img/redirect_in.png" alt="redirect_in" style="zoom:76%;" />

​	(6) `ls -a -l | grep a  `

<img src="http://gitea.shuishan.net.cn/10195501441/OS2021_Project1.Shell/raw/branch/master/lab_report/test_case_img/pipe.png" alt="pipe" style="zoom:76%;" />

​	(7) `vi result.txt &`

<img src="http://gitea.shuishan.net.cn/10195501441/OS2021_Project1.Shell/raw/branch/master/lab_report/test_case_img/vi_bg.png" alt="vi_bg" style="zoom:76%;" />

​	(8) `mytop  `

<img src="http://gitea.shuishan.net.cn/10195501441/OS2021_Project1.Shell/raw/branch/master/lab_report/test_case_img/mytop.png" alt="mytop" style="zoom:76%;" />

​	(9) `history n`

<img src="http://gitea.shuishan.net.cn/10195501441/OS2021_Project1.Shell/raw/branch/master/lab_report/test_case_img/history.png" alt="history" style="zoom:76%;" />

​	(10) `exit`

​	<img src="http://gitea.shuishan.net.cn/10195501441/OS2021_Project1.Shell/raw/branch/master/lab_report/test_case_img/exit.png" alt="exit" style="zoom:76%;" />
#include <stdio.h>
#include <graphics.h> //easyx图形库
#include <time.h>
#include <math.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include "vector2.h"
#include "tools.h"

#define WIN_WIDTH 900
#define WIN_HEIGHT 600
#define ZOMBIE_MAX 20

enum { WAN_DOU, XIANG_RI_KUI, PLANT_COUNT };

IMAGE imgBackground;	//表示背景图片
IMAGE imgBar;	//表示工具栏
IMAGE imgCards[PLANT_COUNT];	//表示植物卡牌
IMAGE* imgPlant[PLANT_COUNT][20];	//表示植物


int curX, curY;	//当前选中的植物，在移动过程中的位置
int curPlant;	// >0:没有选中  1：选择了第一种植物

enum { GOING, WIN, FAIL };
int killCount;	//当前已经击杀的僵尸个数
int zombieCount;	//当前已经出现的僵尸个数
int gameStatus;	//当前游戏的状态

struct plant
{
	int type;	//0:表示没有植物	1：表示第一种植物
	int frameIndex;		//序列帧的序号
	bool catched;	//是否正在被僵尸攻击
	int deadTime;	//表示植物的血量

	int timer;	//植物生产阳光的计时器
	int x, y;

	int shootTime;
};
struct plant map[3][9];
enum {SUNSHINE_DOWN, SUNSHINE_GROUND, SUNSHINE_COLLECT, SUNSHINE_PRODUCT};

struct sunshineBall
{
	int x, y;	//阳光球在飘落过程中的位置（x不变）
	int frameIndex;	//当前显示的图片帧的序号
	int destY;	//飘落的目标位置的Y坐标
	bool used;	//是否在使用
	int timer;

	float xoff;
	float yoff;

	float t;	//贝塞尔曲线的时间点	0..1
	vector2 p1, p2, p3, p4;
	vector2 pCur;	//当前时刻阳光球的位置
	float speed;	//阳光球运动的速度
	int status;		//阳光球当前的状态
};
struct sunshineBall balls[10];
IMAGE imgSunshineBall[29];
int sunshie;	//初始阳光

//创建僵尸
struct zombie
{
	int x, y;	//保存僵尸的坐标
	int frameIndex;	//当前显示的图片帧的序号
	bool used;	//是否在使用
	int speed;	//僵尸的移动速度
	int row;	//保存僵尸所在的行
	int blood;	//保存僵尸的血量
	bool dead;	//判断僵尸是否死亡
	bool eating;
};
struct zombie zombies[10];	
IMAGE imgZombie[22];
IMAGE imgZombieDead[20];
IMAGE imgZombieEat[21];
IMAGE imgZombieStand[11];

//子弹的数据类型
struct bullet
{
	int x, y;
	int row;
	bool used;
	int speed;
	bool blast;	//是否发生爆炸
	int frameIndex;
};
struct bullet bullets[30];
IMAGE imgBulletNormal;
IMAGE imgBulletBlast[4];

bool fileExist(const char* name)
{
	FILE* fp = fopen(name, "r");
	if (fp == NULL)
	{
		return false;
	}
	else
	{
		fclose(fp);
		return true;
	}
}

void gameInit()
{
	loadimage(&imgBackground, "res/bg.jpg");	//加载背景图片（把字符集修改为“多字节字符集”）
	loadimage(&imgBar, "res/bar5.png");

	memset(imgPlant, 0, sizeof(imgPlant));
	memset(map, 0, sizeof(map));

	killCount = 0;
	zombieCount = 0;
	gameStatus = GOING;

	//初始化植物卡牌
	char name[64];
	for (int i = 0; i < PLANT_COUNT; i++)
	{
		//生成植物卡牌的文件名
		sprintf_s(name, sizeof(name), "res/Cards/card_%d.png", i + 1);
		loadimage(&imgCards[i], name);

		for (int j = 0; j < 20; j++)
		{
			sprintf_s(name, sizeof(name), "res/zhiwu/%d/%d.png", i, j + 1);
			//先判断这个文件是否存在
			if (fileExist(name))
			{
				imgPlant[i][j] = new IMAGE;	//分配一个内存
				loadimage(imgPlant[i][j], name);	//加载植物
			}
			else
			{
				break;	//若文件不存在，则跳出循环
			}
		}
	}

	curPlant = 0;
	sunshie = 50;

	memset(balls, 0, sizeof(balls));
	for (int i = 0; i < 29; i++)
	{
		sprintf_s(name, sizeof(name), "res/sunshine/%d.png", i + 1);
		loadimage(&imgSunshineBall[i], name);
	}

	srand(time(NULL));//配置随机数种子

	initgraph(WIN_WIDTH, WIN_HEIGHT, 1);	//创建图形窗口

	//设置字体
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 30;
	f.lfWeight = 15;
	strcpy(f.lfFaceName, "Segoe UI Black");
	f.lfQuality = ANTIALIASED_QUALITY;//抗锯齿效果
	settextstyle(&f);
	setbkmode(TRANSPARENT);//设置字体背景模式为透明
	setcolor(BLACK);

	//初始化僵尸数据
	memset(zombies, 0, sizeof(zombies));
	for (int i = 0; i < 22; i++)
	{
		sprintf_s(name, sizeof(name), "res/zm/%d.png", i + 1);
		loadimage(&imgZombie[i], name);
	}

	loadimage(&imgBulletNormal, "res/bullets/bullet_normal.png");
	memset(bullets, 0, sizeof(bullets));

	//初始化豌豆子弹的帧图片数组
	loadimage(&imgBulletBlast[3], "res/bullets/bullet_blast.png");
	for (int i = 0; i < 3; i++)
	{
		float k = (i + 1) * 0.2;
		loadimage(&imgBulletBlast[i], "res/bullets/bullet_blast.png",
			imgBulletBlast[3].getwidth() * k, imgBulletBlast[3].getheight() * k, true);
	}

	//初始化僵尸死亡的图片
	for (int i = 0; i < 20; i++)
	{
		sprintf_s(name, sizeof(name), "res/zm_dead/%d.png", i + 1);
		loadimage(&imgZombieDead[i], name);
	}

	//初始化僵尸吃植物的图片
	for (int i = 0; i < 21; i++)
	{
		sprintf_s(name, "res/zm_eat/%d.png", i + 1);
		loadimage(&imgZombieEat[i], name);
	}

	//初始化僵尸站立的图片
	for (int i = 0; i < 11; i++)
	{
		sprintf_s(name, "res/zm_stand/%d.png", i + 1);
		loadimage(&imgZombieStand[i], name);
	}
}

void  drawSunshines()
{
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++)
	{
		if (balls[i].used)
		{
			IMAGE* img = &imgSunshineBall[balls[i].frameIndex];
			putimagePNG(balls[i].pCur.x, balls[i].pCur.y, img);
		}
	}
}

void drawCards()
{
	for (int i = 0; i < PLANT_COUNT; i++)
	{
		int x = 338 + i * 65;
		int y = 6;
		putimage(x, y, &imgCards[i]);
	}
}

void drawPlant()
{
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type > 0)
			{
				int zhiWuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				putimagePNG(map[i][j].x, map[i][j].y, imgPlant[zhiWuType][index]);
			}
		}
	}

	//渲染 拖动过程中的植物
	if (curPlant > 0)
	{
		IMAGE* img = imgPlant[curPlant - 1][0];
		putimagePNG(curX - img->getwidth() / 2, curY - img->getwidth() / 2, img);
	}
}

void drawZombie()
{
	int zmCount = sizeof(zombies) / sizeof(zombies[0]);
	for (int i = 0; i < zmCount; i++)
	{
		if (zombies[i].used)
		{
			//定义一个空的指针，若僵尸死亡为真，则指向imgZMDead数组，若僵尸攻击为真，则指向imgZMEat数组；若都为假，则指向imgZM数组
			IMAGE* img = NULL;
			if (zombies[i].dead)
			{
				img = imgZombieDead;
			}
			else if (zombies[i].eating)
			{
				img = imgZombieEat;
			}
			else
			{
				img = imgZombie;
			}

			img += zombies[i].frameIndex;
			putimagePNG(zombies[i].x, zombies[i].y - img->getheight(), img);
		}
	}
}

void drawBullets()
{
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < bulletMax; i++)
	{
		//判断子弹是否使用
		if (bullets[i].used)
		{
			if (bullets[i].blast)
			{
				//如处于爆炸状态则渲染此处
				putimagePNG(bullets[i].x, bullets[i].y, &imgBulletBlast[bullets[i].frameIndex]);
			}
			else
			{
				//如处于正常状态则渲染此处
				putimagePNG(bullets[i].x, bullets[i].y, &imgBulletNormal);
			}
		}
	}
}

void updateWindow()
{
	BeginBatchDraw();	//开始双缓冲

	putimage(0, 0, &imgBackground);	//渲染背景图片
	putimagePNG(250, 0, &imgBar);	//渲染工具栏

	drawCards();	//渲染植物卡牌
	drawPlant();	//渲染 被种植的植物
	drawSunshines();	//渲染阳关球
	drawZombie();	//渲染僵尸
	drawBullets();	//渲染豌豆的子弹

	char scoreText[8];
	sprintf_s(scoreText, sizeof(scoreText), "%d", sunshie);
	outtextxy(281, 67, scoreText);	//输出分数

	EndBatchDraw();		//结束双缓冲
}

void collectSunshine(ExMessage* msg)
{
	int count = sizeof(balls) / sizeof(balls[0]);
	int w = imgSunshineBall[0].getwidth();
	int h = imgSunshineBall[0].getheight();
	for (int i = 0; i < count; i++)
	{
		if (balls[i].used)
		{
			//定义阳光球当前的位置
			int x = balls[i].pCur.x;
			int y = balls[i].pCur.y;

			if (msg->x > x && msg->x < x + w && msg->y > y && msg->y < y + h)
			{
				balls[i].status = SUNSHINE_COLLECT;
				mciSendString("play res/sunshine.mp3", 0, 0, 0);
				/*PlaySound("res/sunshine.wav", NULL, SND_FILENAME | SND_ASYNC);*/   //.wav文件播放方式 间隔更短
				//设置阳光的偏移量
				/*float destX = 262;
				float destY = 0;
				float angle = atan((y - destY) / (x - destX));
				balls[i].xoff = 4 * cos(angle);
				balls[i].yoff = 4 * sin(angle);*/


				balls[i].p1 = balls[i].pCur;	//阳光球位置的起点
				balls[i].p4 = vector2(262, 0);	//阳光球位置的终点
				balls[i].t = 0;
				float distance = dis(balls[i].p1 - balls[i].p4);	//求起点与终点的模长（阳光球起点与终点的距离）
				float off = 8;	//阳光球每次移动的距离（控制阳光球移动的速度）
				balls[i].speed = 1.0 / (distance / off);
				break;
			}
		}
	}
}

void userClick()
{
	ExMessage msg;
	static int status = 0;
	if (peekmessage(&msg))
	{
		if (msg.message == WM_LBUTTONDOWN)
		{
			if (msg.x > 338 && msg.x < 338 + 65 * PLANT_COUNT && msg.y < 96)
			{
				int index = (msg.x - 338) / 65;
				status = 1;
				curPlant = index + 1;
			}
			else
			{
				collectSunshine(&msg);	//收集阳光
			}
		}
		else if (msg.message == WM_MOUSEMOVE && status == 1)
		{
			curX = msg.x;
			curY = msg.y;
		}
		else if (msg.message == WM_LBUTTONUP)
		{
			//判断草坪的边界值
			if (msg.x > 256 && msg.y > 179 && msg.y < 489)
			{
				int row = (msg.y - 179) / 102;
				int col = (msg.x - 256) / 81;

				//判断该位置是否已经种植植物
				if (map[row][col].type == 0)
				{
					map[row][col].type = curPlant;
					map[row][col].frameIndex = 0;
					map[row][col].shootTime = 0;

					map[row][col].x = 256 + col * 81;
					map[row][col].y = 179 + row * 102 + 14;
				}
			}

			curPlant = 0;
			status = 0;
		}
	}
}

void createSunshine()
{
	//
	static int count = 0;
	static int fre = 400;
	count++;
	if (count > fre)
	{
		fre = 200 + rand() % 200;
		count = 0;
		//从阳光池里取一个可以使用的
		int ballMax = sizeof(balls) / sizeof(balls[0]);

		int i;
		for (i = 0; i < ballMax && balls[i].used; i++);
		if (i >= ballMax)return;

		balls[i].used = true;
		balls[i].frameIndex = 0;
		balls[i].timer = 0;


		balls[i].status = SUNSHINE_DOWN;
		balls[i].t = 0;
		balls[i].p1 = vector2(260 + rand() % (900 - 260), 60);
		balls[i].p4 = vector2(balls[i].p1.x, 200 + (rand() % 4) * 90);
		float distance = balls[i].p4.y - balls[i].p1.y;
		int off = 2;
		balls[i].speed = 1.0 / (distance / off);
	}

	//生产阳光
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type == XIANG_RI_KUI + 1)
			{
				map[i][j].timer++;
				if (map[i][j].timer > 200)
				{
					map[i][j].timer = 0;

					int k;
					for (k = 0; k < ballMax && balls[k].used; k++);
					if (k >= ballMax) return;

					balls[k].used = true;
					balls[k].p1 = vector2(map[i][j].x, map[i][j].y);
					int w = (100 + rand() % 50) * (rand() % 2 ? 1 : -1);
					balls[k].p4 = vector2(map[i][j].x + w,
						map[i][j].y + imgPlant[XIANG_RI_KUI][0]->getheight() - 
						imgSunshineBall[0].getheight());
					balls[k].p2 = vector2(balls[k].p1.x + w * 0.3, balls[k].p1.y - 100);
					balls[k].p3 = vector2(balls[k].p1.x + w * 0.7, balls[k].p1.y - 100);
					balls[k].status = SUNSHINE_PRODUCT;
					balls[k].speed = 0.05;
					balls[k].t = 0;
				}
			}
		}
	}
}

void updateSunshine()
{
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++)
	{
		if (balls[i].used)
		{
			balls[i].frameIndex = (balls[i].frameIndex + 1) % 29;
			if (balls[i].status == SUNSHINE_DOWN)
			{
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);
				if (sun->t >= 1)
				{
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_GROUND)
			{
				balls[i].timer++;
				if (balls[i].timer > 100)
				{
					balls[i].used = false;
					balls[i].timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_COLLECT)
			{
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);
				if (sun->t > 1)
				{
					sun->used = false;
					sunshie += 25;
				}
			}
			else if (balls[i].status == SUNSHINE_PRODUCT)
			{
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = calcBezierPoint(sun->t, sun->p1, sun->p2, sun->p3, sun->p4);
				if (sun->t > 1)
				{
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}
		}
	}
}

void createZombie()
{
	if (zombieCount >= ZOMBIE_MAX)
	{
		return;
	}

	//定义僵尸生成间隔
	static int zombieFre = 100;
	static int count = 0;
	count++;
	if (count > zombieFre)
	{
		count = 0;
		zombieFre = rand() % 200 + 100;

		//定义生成的僵尸
		int i;
		int zombieMax = sizeof(zombies) / sizeof(zombies[0]);
		for (i = 0; i < zombieMax && zombies[i].used; i++);
		if (i < zombieMax)
		{
			memset(&zombies[i], 0, sizeof(zombies[i]));
			zombies[i].used = true;
			zombies[i].x = WIN_WIDTH;
			zombies[i].row = rand() % 3;
			zombies[i].y = 172 + (1 + zombies[i].row) * 100;
			zombies[i].speed = 1;
			zombies[i].blood = 100;
			zombies[i].dead = false;
			zombieCount++;
		}
		else
		{
			printf("创建僵尸失败\n");
		}
	}
}

void updateZombie()
{
	int zombieMax = sizeof(zombies) / sizeof(zombies[0]);

	static int count = 0;
	count++;
	if (count > 2)	//控制僵尸移动的速度
	{
		count = 0;
		//更新僵尸的位置
		for (int i = 0; i < zombieMax; i++)
		{
			if (zombies[i].used)
			{
				zombies[i].x -= zombies[i].speed;
				//判断僵尸是否跨进房子
				if (zombies[i].x < 56)
				{
					gameStatus = FAIL;
				}
			}
		}
	}

	static int count2 = 0;
	count2++;
	if (count2 > 4)
	{
		count2 = 0;
		for (int i = 0; i < zombieMax; i++)
		{
			if (zombies[i].used)
			{
				//判断僵尸是死亡
				if (zombies[i].dead)
				{
					zombies[i].frameIndex++;
					if (zombies[i].frameIndex >= 20)
					{
						zombies[i].used = false;
						killCount++;
						if (killCount == ZOMBIE_MAX)
						{
							gameStatus = WIN;
						}
					}
				}
				else if (zombies[i].eating)
				{
					//僵尸攻击（21帧）
					zombies[i].frameIndex = (zombies[i].frameIndex + 1) % 21;
				}
				else {
					//僵尸行走（22帧）
					zombies[i].frameIndex = (zombies[i].frameIndex + 1) % 22;
				}
			}
		}
	}
}

void shoot()
{
	//控制子弹发射的频率
	static int count = 0;
	if (++count < 1)return;
	count = 0;


	int lines[3] = { 0 };
	int zmCount = sizeof(zombies) / sizeof(zombies[0]);
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	int dangerX = WIN_WIDTH - imgZombie[0].getwidth();
	for (int i = 0; i < zmCount; i++)
	{
		if (zombies[i].used && zombies[i].x < dangerX)
		{
			lines[zombies[i].row] = 1;
		}
	}

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type == WAN_DOU + 1 && lines[i])
			{
				map[i][j].shootTime++;
				if (map[i][j].shootTime > 50)
				{
					map[i][j].shootTime = 0;

					int k;
					for (k = 0; k < bulletMax && bullets[k].used; k++);
					if (k < bulletMax)
					{
						bullets[k].used = true;
						bullets[k].row = i;
						bullets[k].speed = 4;
						bullets[k].blast = false;
						bullets[k].frameIndex = 0;
						
						//定位子弹发射的位置（豌豆的口中）
						int zwX = 256 + j * 81;
						int zwY = 179 + i * 102 + 14;
						bullets[k].x = zwX + imgPlant[map[i][j].type - 1][0]->getwidth() - 10;
						bullets[k].y = zwY + 5;
					}
				}
			}
		}
	}
}

void updateBullets()
{
	int countMax = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < countMax; i++)
	{
		if (bullets[i].used)
		{
			bullets[i].x += bullets[i].speed;
			if (bullets[i].x > WIN_WIDTH)
			{
				bullets[i].used = false;
			}
			//完善子弹的碰撞检测
			if (bullets[i].blast)
			{
				bullets[i].frameIndex++;
				if (bullets[i].frameIndex >= 4)
				{
					bullets[i].used = false;
				}
			}
		}
	}
}

void checkBulletToZm()
{
	int bulletCount = sizeof(bullets) / sizeof(bullets[0]);
	int zmCount = sizeof(zombies) / sizeof(zombies[0]);
	for (int i = 0; i < bulletCount; i++)
	{
		if (bullets[i].used == false || bullets[i].blast)continue;//判断子弹是否存在

		for (int k = 0; k < zmCount; k++)
		{
			if (zombies[k].used == false) continue;//判断僵尸是否存在
			//定义僵尸素材大小的范围区间
			int x1 = zombies[k].x + 80;
			int x2 = zombies[k].x + 110;

			int x = bullets[i].x;
			//判断子弹和僵尸是否在同一行，再判断子弹的坐标是否在僵尸坐标的范围内
			if (zombies[k].dead == false && bullets[i].row == zombies[k].row && x > x1 && x < x2)
			{
				zombies[k].blood -= 10;
				bullets[i].blast = true;	//子弹变为爆炸状态
				bullets[i].speed = 0;

				if (zombies[k].blood <= 0)
				{
					zombies[k].dead = true;
					zombies[k].speed = 0;
					zombies[k].frameIndex = 0;
				}
				//豌豆子弹已经与僵尸完成了碰撞检测，则跳出循环
				break;
			}
		}
	}
}

void checkZmToZhiWu()
{
	int zmCount = sizeof(zombies) / sizeof(zombies[0]);
	for (int i = 0; i < zmCount; i++)
	{
		if (zombies[i].dead)continue;

		int row = zombies[i].row;
		for (int k = 0; k < 9; k++)
		{
			if (map[row][k].type == 0)continue;	//若判断该行没有植物，则继续循环
			
			int plant_X = 256 + k * 81;	//植物的坐标
			int left_X = plant_X + 10;	//该植物素材左边界的坐标
			int right_X = plant_X + 60;	//该植物素材右边界的坐标
			int zm_X = zombies[i].x + 80;	//该僵尸与植物碰撞的边界的坐标

			//判断僵尸是否与植物发生碰撞
			if (zm_X > left_X && zm_X < right_X)
			{
				if (map[row][k].catched)
				{
					map[row][k].deadTime++;
					if (map[row][k].deadTime > 100)
					{
						map[row][k].deadTime = 0;
						map[row][k].type = 0;
						zombies[i].eating = false;
						zombies[i].frameIndex = 0;
						zombies[i].speed = 1;
					}
				}
				else
				{
					map[row][k].catched = true;
					map[row][k].deadTime = 0;
					zombies[i].eating = true;
					zombies[i].speed = 0;
					zombies[i].frameIndex = 0;
				}
			}
		}
	}
}

void collisionCheck()
{
	checkBulletToZm();	//子弹对僵尸的碰撞检测
	checkZmToZhiWu();	//僵尸对植物的碰撞检测
}

void updatePlant()
{
	static int count = 0;
	if (++count < 2)return;
	count = 0;

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type > 0)
			{
				map[i][j].frameIndex++;
				int zhiWuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				if (imgPlant[zhiWuType][index] == NULL)
				{
					map[i][j].frameIndex = 0;
				}
			}
		}
	}
}

void updateGame()
{
	updatePlant();	//更新植物
	createSunshine();//创建阳光
	updateSunshine();//更新阳光的状态
	createZombie();//创建僵尸
	updateZombie();//更新僵尸的状态
	shoot();//发射豌豆子弹
	updateBullets();//更新豌豆的子弹
	collisionCheck();//实现豌豆子弹和僵尸的碰撞检测
}

void startUI()
{
	IMAGE imgMenu, imgMenu1, imgMenu2;
	loadimage(&imgMenu, "res/menu.png");
	loadimage(&imgMenu1, "res/menu1.png");
	loadimage(&imgMenu2, "res/menu2.png");
	int flag = 0;

	while (1)
	{
		BeginBatchDraw();
		putimage(0, 0, &imgMenu);
		putimagePNG(474, 75, flag ? &imgMenu2 : &imgMenu1);

		ExMessage msg;
		if (peekmessage(&msg))
		{
			if (msg.message == WM_LBUTTONDOWN && msg.x > 474 && msg.x < 474 + 330 && msg.y>75 && msg.y < 75 + 140)
			{
				flag = 1;
			}
			else if (msg.message == WM_LBUTTONUP && flag)
			{
				mciSendString("play res/1921.mp3", 0, 0, 0);
				EndBatchDraw();
				break;
			}
		}

		EndBatchDraw();
	}
}

void viewScence()
{
	int xMin = WIN_WIDTH - imgBackground.getwidth();	//900-1400=-500
	//定义开场僵尸站的位置
	vector2 points[9] =
	{
		{550,80},{530,160},{630,170},{530,200},{515,270},
		{565,370},{605,340},{705,280},{690,340}
	};
	//定义开场僵尸站立的图片帧为随机
	int index[9];
	for (int i = 0; i < 9; i++)
	{
		index[i] = rand() % 11;
	}

	int count = 0;
	for (int x = 0; x >= xMin; x -= 2)
	{
		BeginBatchDraw();
		putimage(x, 0, &imgBackground);

		count++;
		for (int k = 0; k < 9; k++)
		{
			putimagePNG(points[k].x - xMin + x, points[k].y, &imgZombieStand[index[k]]);
			//定义僵尸站立帧的速度
			if (count >= 4)
			{
				index[k] = (index[k] + 1) % 11;
			}
		}
		if (count >= 10)
		{
			count = 0;
		}

		EndBatchDraw();
		Sleep(5);
	}

	//设置在该界面的停留时间
	for (int i = 0; i < 100; i++)
	{
		BeginBatchDraw();

		putimage(xMin, 0, &imgBackground);
		count++;
		for (int k = 0; k < 9; k++)
		{
			putimagePNG(points[k].x, points[k].y, &imgZombieStand[index[k]]);
			index[k] = (index[k] + 1) % 11;
			if (count >= 4)
			{
				index[k] = (index[k] + 1) % 11;
			}
			if (count >= 10)
			{
				count = 0;
			}
		}
		EndBatchDraw();
		Sleep(30);
	}

	//场景回转
	for (int x = xMin; x <= 0; x += 2)
	{
		BeginBatchDraw();

		putimage(x, 0, &imgBackground);

		count++;
		for (int k = 0; k < 9; k++)
		{
			putimagePNG(points[k].x - xMin + x, points[k].y, &imgZombieStand[index[k]]);
			if (count >= 4)
			{
				index[k] = (index[k] + 1) % 11;
			}
			if (count >= 10)
			{
				count = 0;
			}

		}
		EndBatchDraw();
		Sleep(5);
	}
}

void barsDown()
{
	int height = imgBar.getheight();
	for (int y = -height; y <= 0; y++)
	{
		BeginBatchDraw();

		putimage(0, 0, &imgBackground);
		putimagePNG(250, y, &imgBar);

		for (int i = 0; i < PLANT_COUNT; i++)
		{
			int x = 338 + i * 65;
			putimage(x, y + 6, &imgCards[i]);
		}

		EndBatchDraw();
		Sleep(5);
	}
}

bool checkOver()
{
	int ret = false;
	if (gameStatus == WIN)
	{
		Sleep(2000);
		loadimage(0, "res/gameWin.png");
		ret = true;
	}
	else if (gameStatus == FAIL)
	{
		Sleep(2000);
		loadimage(0, "res/gameFail.png");
		ret = true;
	}

	return ret;
}

int main()
{
	gameInit();
	startUI();
	viewScence();
	barsDown();

	int timer = 0;
	bool flag = true;
	
	while (1)
	{
		userClick();

		//游戏帧刷新
		timer += getDelay();
		if (timer > 10)
		{
			flag = true;
			timer = 0;
		}
		if (flag)
		{
			flag = false;
			updateWindow();
			updateGame();
			if (checkOver())break;
		}
	}

	system("pause");
}
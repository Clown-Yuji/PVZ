#include <stdio.h>
#include <graphics.h> //easyxͼ�ο�
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

IMAGE imgBackground;	//��ʾ����ͼƬ
IMAGE imgBar;	//��ʾ������
IMAGE imgCards[PLANT_COUNT];	//��ʾֲ�￨��
IMAGE* imgPlant[PLANT_COUNT][20];	//��ʾֲ��


int curX, curY;	//��ǰѡ�е�ֲ����ƶ������е�λ��
int curPlant;	// >0:û��ѡ��  1��ѡ���˵�һ��ֲ��

enum { GOING, WIN, FAIL };
int killCount;	//��ǰ�Ѿ���ɱ�Ľ�ʬ����
int zombieCount;	//��ǰ�Ѿ����ֵĽ�ʬ����
int gameStatus;	//��ǰ��Ϸ��״̬

struct plant
{
	int type;	//0:��ʾû��ֲ��	1����ʾ��һ��ֲ��
	int frameIndex;		//����֡�����
	bool catched;	//�Ƿ����ڱ���ʬ����
	int deadTime;	//��ʾֲ���Ѫ��

	int timer;	//ֲ����������ļ�ʱ��
	int x, y;

	int shootTime;
};
struct plant map[3][9];
enum {SUNSHINE_DOWN, SUNSHINE_GROUND, SUNSHINE_COLLECT, SUNSHINE_PRODUCT};

struct sunshineBall
{
	int x, y;	//��������Ʈ������е�λ�ã�x���䣩
	int frameIndex;	//��ǰ��ʾ��ͼƬ֡�����
	int destY;	//Ʈ���Ŀ��λ�õ�Y����
	bool used;	//�Ƿ���ʹ��
	int timer;

	float xoff;
	float yoff;

	float t;	//���������ߵ�ʱ���	0..1
	vector2 p1, p2, p3, p4;
	vector2 pCur;	//��ǰʱ���������λ��
	float speed;	//�������˶����ٶ�
	int status;		//������ǰ��״̬
};
struct sunshineBall balls[10];
IMAGE imgSunshineBall[29];
int sunshie;	//��ʼ����

//������ʬ
struct zombie
{
	int x, y;	//���潩ʬ������
	int frameIndex;	//��ǰ��ʾ��ͼƬ֡�����
	bool used;	//�Ƿ���ʹ��
	int speed;	//��ʬ���ƶ��ٶ�
	int row;	//���潩ʬ���ڵ���
	int blood;	//���潩ʬ��Ѫ��
	bool dead;	//�жϽ�ʬ�Ƿ�����
	bool eating;
};
struct zombie zombies[10];	
IMAGE imgZombie[22];
IMAGE imgZombieDead[20];
IMAGE imgZombieEat[21];
IMAGE imgZombieStand[11];

//�ӵ�����������
struct bullet
{
	int x, y;
	int row;
	bool used;
	int speed;
	bool blast;	//�Ƿ�����ը
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
	loadimage(&imgBackground, "res/bg.jpg");	//���ر���ͼƬ�����ַ����޸�Ϊ�����ֽ��ַ�������
	loadimage(&imgBar, "res/bar5.png");

	memset(imgPlant, 0, sizeof(imgPlant));
	memset(map, 0, sizeof(map));

	killCount = 0;
	zombieCount = 0;
	gameStatus = GOING;

	//��ʼ��ֲ�￨��
	char name[64];
	for (int i = 0; i < PLANT_COUNT; i++)
	{
		//����ֲ�￨�Ƶ��ļ���
		sprintf_s(name, sizeof(name), "res/Cards/card_%d.png", i + 1);
		loadimage(&imgCards[i], name);

		for (int j = 0; j < 20; j++)
		{
			sprintf_s(name, sizeof(name), "res/zhiwu/%d/%d.png", i, j + 1);
			//���ж�����ļ��Ƿ����
			if (fileExist(name))
			{
				imgPlant[i][j] = new IMAGE;	//����һ���ڴ�
				loadimage(imgPlant[i][j], name);	//����ֲ��
			}
			else
			{
				break;	//���ļ������ڣ�������ѭ��
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

	srand(time(NULL));//�������������

	initgraph(WIN_WIDTH, WIN_HEIGHT, 1);	//����ͼ�δ���

	//��������
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 30;
	f.lfWeight = 15;
	strcpy(f.lfFaceName, "Segoe UI Black");
	f.lfQuality = ANTIALIASED_QUALITY;//�����Ч��
	settextstyle(&f);
	setbkmode(TRANSPARENT);//�������屳��ģʽΪ͸��
	setcolor(BLACK);

	//��ʼ����ʬ����
	memset(zombies, 0, sizeof(zombies));
	for (int i = 0; i < 22; i++)
	{
		sprintf_s(name, sizeof(name), "res/zm/%d.png", i + 1);
		loadimage(&imgZombie[i], name);
	}

	loadimage(&imgBulletNormal, "res/bullets/bullet_normal.png");
	memset(bullets, 0, sizeof(bullets));

	//��ʼ���㶹�ӵ���֡ͼƬ����
	loadimage(&imgBulletBlast[3], "res/bullets/bullet_blast.png");
	for (int i = 0; i < 3; i++)
	{
		float k = (i + 1) * 0.2;
		loadimage(&imgBulletBlast[i], "res/bullets/bullet_blast.png",
			imgBulletBlast[3].getwidth() * k, imgBulletBlast[3].getheight() * k, true);
	}

	//��ʼ����ʬ������ͼƬ
	for (int i = 0; i < 20; i++)
	{
		sprintf_s(name, sizeof(name), "res/zm_dead/%d.png", i + 1);
		loadimage(&imgZombieDead[i], name);
	}

	//��ʼ����ʬ��ֲ���ͼƬ
	for (int i = 0; i < 21; i++)
	{
		sprintf_s(name, "res/zm_eat/%d.png", i + 1);
		loadimage(&imgZombieEat[i], name);
	}

	//��ʼ����ʬվ����ͼƬ
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

	//��Ⱦ �϶������е�ֲ��
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
			//����һ���յ�ָ�룬����ʬ����Ϊ�棬��ָ��imgZMDead���飬����ʬ����Ϊ�棬��ָ��imgZMEat���飻����Ϊ�٣���ָ��imgZM����
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
		//�ж��ӵ��Ƿ�ʹ��
		if (bullets[i].used)
		{
			if (bullets[i].blast)
			{
				//�紦�ڱ�ը״̬����Ⱦ�˴�
				putimagePNG(bullets[i].x, bullets[i].y, &imgBulletBlast[bullets[i].frameIndex]);
			}
			else
			{
				//�紦������״̬����Ⱦ�˴�
				putimagePNG(bullets[i].x, bullets[i].y, &imgBulletNormal);
			}
		}
	}
}

void updateWindow()
{
	BeginBatchDraw();	//��ʼ˫����

	putimage(0, 0, &imgBackground);	//��Ⱦ����ͼƬ
	putimagePNG(250, 0, &imgBar);	//��Ⱦ������

	drawCards();	//��Ⱦֲ�￨��
	drawPlant();	//��Ⱦ ����ֲ��ֲ��
	drawSunshines();	//��Ⱦ������
	drawZombie();	//��Ⱦ��ʬ
	drawBullets();	//��Ⱦ�㶹���ӵ�

	char scoreText[8];
	sprintf_s(scoreText, sizeof(scoreText), "%d", sunshie);
	outtextxy(281, 67, scoreText);	//�������

	EndBatchDraw();		//����˫����
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
			//����������ǰ��λ��
			int x = balls[i].pCur.x;
			int y = balls[i].pCur.y;

			if (msg->x > x && msg->x < x + w && msg->y > y && msg->y < y + h)
			{
				balls[i].status = SUNSHINE_COLLECT;
				mciSendString("play res/sunshine.mp3", 0, 0, 0);
				/*PlaySound("res/sunshine.wav", NULL, SND_FILENAME | SND_ASYNC);*/   //.wav�ļ����ŷ�ʽ �������
				//���������ƫ����
				/*float destX = 262;
				float destY = 0;
				float angle = atan((y - destY) / (x - destX));
				balls[i].xoff = 4 * cos(angle);
				balls[i].yoff = 4 * sin(angle);*/


				balls[i].p1 = balls[i].pCur;	//������λ�õ����
				balls[i].p4 = vector2(262, 0);	//������λ�õ��յ�
				balls[i].t = 0;
				float distance = dis(balls[i].p1 - balls[i].p4);	//��������յ��ģ����������������յ�ľ��룩
				float off = 8;	//������ÿ���ƶ��ľ��루�����������ƶ����ٶȣ�
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
				collectSunshine(&msg);	//�ռ�����
			}
		}
		else if (msg.message == WM_MOUSEMOVE && status == 1)
		{
			curX = msg.x;
			curY = msg.y;
		}
		else if (msg.message == WM_LBUTTONUP)
		{
			//�жϲ�ƺ�ı߽�ֵ
			if (msg.x > 256 && msg.y > 179 && msg.y < 489)
			{
				int row = (msg.y - 179) / 102;
				int col = (msg.x - 256) / 81;

				//�жϸ�λ���Ƿ��Ѿ���ֲֲ��
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
		//���������ȡһ������ʹ�õ�
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

	//��������
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

	//���彩ʬ���ɼ��
	static int zombieFre = 100;
	static int count = 0;
	count++;
	if (count > zombieFre)
	{
		count = 0;
		zombieFre = rand() % 200 + 100;

		//�������ɵĽ�ʬ
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
			printf("������ʬʧ��\n");
		}
	}
}

void updateZombie()
{
	int zombieMax = sizeof(zombies) / sizeof(zombies[0]);

	static int count = 0;
	count++;
	if (count > 2)	//���ƽ�ʬ�ƶ����ٶ�
	{
		count = 0;
		//���½�ʬ��λ��
		for (int i = 0; i < zombieMax; i++)
		{
			if (zombies[i].used)
			{
				zombies[i].x -= zombies[i].speed;
				//�жϽ�ʬ�Ƿ�������
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
				//�жϽ�ʬ������
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
					//��ʬ������21֡��
					zombies[i].frameIndex = (zombies[i].frameIndex + 1) % 21;
				}
				else {
					//��ʬ���ߣ�22֡��
					zombies[i].frameIndex = (zombies[i].frameIndex + 1) % 22;
				}
			}
		}
	}
}

void shoot()
{
	//�����ӵ������Ƶ��
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
						
						//��λ�ӵ������λ�ã��㶹�Ŀ��У�
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
			//�����ӵ�����ײ���
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
		if (bullets[i].used == false || bullets[i].blast)continue;//�ж��ӵ��Ƿ����

		for (int k = 0; k < zmCount; k++)
		{
			if (zombies[k].used == false) continue;//�жϽ�ʬ�Ƿ����
			//���彩ʬ�زĴ�С�ķ�Χ����
			int x1 = zombies[k].x + 80;
			int x2 = zombies[k].x + 110;

			int x = bullets[i].x;
			//�ж��ӵ��ͽ�ʬ�Ƿ���ͬһ�У����ж��ӵ��������Ƿ��ڽ�ʬ����ķ�Χ��
			if (zombies[k].dead == false && bullets[i].row == zombies[k].row && x > x1 && x < x2)
			{
				zombies[k].blood -= 10;
				bullets[i].blast = true;	//�ӵ���Ϊ��ը״̬
				bullets[i].speed = 0;

				if (zombies[k].blood <= 0)
				{
					zombies[k].dead = true;
					zombies[k].speed = 0;
					zombies[k].frameIndex = 0;
				}
				//�㶹�ӵ��Ѿ��뽩ʬ�������ײ��⣬������ѭ��
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
			if (map[row][k].type == 0)continue;	//���жϸ���û��ֲ������ѭ��
			
			int plant_X = 256 + k * 81;	//ֲ�������
			int left_X = plant_X + 10;	//��ֲ���ز���߽������
			int right_X = plant_X + 60;	//��ֲ���ز��ұ߽������
			int zm_X = zombies[i].x + 80;	//�ý�ʬ��ֲ����ײ�ı߽������

			//�жϽ�ʬ�Ƿ���ֲ�﷢����ײ
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
	checkBulletToZm();	//�ӵ��Խ�ʬ����ײ���
	checkZmToZhiWu();	//��ʬ��ֲ�����ײ���
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
	updatePlant();	//����ֲ��
	createSunshine();//��������
	updateSunshine();//���������״̬
	createZombie();//������ʬ
	updateZombie();//���½�ʬ��״̬
	shoot();//�����㶹�ӵ�
	updateBullets();//�����㶹���ӵ�
	collisionCheck();//ʵ���㶹�ӵ��ͽ�ʬ����ײ���
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
	//���忪����ʬվ��λ��
	vector2 points[9] =
	{
		{550,80},{530,160},{630,170},{530,200},{515,270},
		{565,370},{605,340},{705,280},{690,340}
	};
	//���忪����ʬվ����ͼƬ֡Ϊ���
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
			//���彩ʬվ��֡���ٶ�
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

	//�����ڸý����ͣ��ʱ��
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

	//������ת
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

		//��Ϸ֡ˢ��
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
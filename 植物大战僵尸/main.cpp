/*	开发日志
*	1.创建空项目模板
*	2.导入素材
*	3.实现最开始的游戏场景
*	4.实现游戏顶部的工具栏
*	5.实现工具栏中的植物卡牌
*	6.实现植物的选择和拖动
*	7.实现植物的摇摆
*/

#include <stdio.h>
#include <graphics.h>
#include <time.h>
#include <math.h>
#include "tools.h"
#include "vector2.h"

#include <mmsystem.h>
#pragma comment(lib,"winmm.lib");

#define SCREEN_WIDHT  900
#define SCREEN_HEIGHT 600
#define ZM_MAX		  10

enum { Pea, sunFlower, Nut, plantCount };

IMAGE imgBg;	//表示背景图片
IMAGE imgBar;
IMAGE Shovel;
IMAGE ShovelSlot;
IMAGE imgPlantCards[plantCount];
IMAGE* imgPlant[plantCount][20];

int curX, curY;	//当前选中的植物，在移动过程中的位置
int curPlant;	//0表示没有选中，1表示选中了第一种植物

enum { GOING, WIN, FAIL };
int killCount;	//当前已经杀掉僵尸的个数
int zmCount;	//当前已经出现僵尸的个数
int gameStatus;

struct plant
{
	int type;	//0表示没有植物，1.第一种植物
	int frameIndex;	//序列帧的序号
	bool catched;	//是否被僵尸捕获
	int blood;

	int timer;	//定时器
	int x, y;
	int shootTimer;		//发射计时器
};

struct plant map[3][9];
enum { SUNSHINE_DOWN, SUNSHINE_GROUND, SUNSHINE_COLLECT, SUNSHINE_PRODUCE };

struct sunshineBall
{
	int x, y;		//阳光球在飘落过程中的坐标位置（X不变）
	int frameIndex;	//当前显示图片帧的序号
	int destY;		//飘落的目标位置的y坐标
	bool used;		//是否在使用
	int timer;		//阳光球的停顿时间(地面着陆停留时间)

	float xoff;		//x偏移量
	float yoff;		//y偏移量

	float t;		//贝塞尔曲线的时间点 0到1
	vector2 p1, p2, p3, p4;		//p1是起点，p4是终点，p2p3是控制点
	vector2 pCur;	//当前时刻阳光球的位置
	float speed;
	int status;		//当前状态
};
struct sunshineBall balls[10];
IMAGE imgSunshineBall[29];
int sunshine;

struct Zombie
{
	int x, y;
	int frameIndex;
	bool used;
	int speed;
	int row;
	int blood;
	bool eating;		//正在吃食物状态
	bool dead;
	bool catching;	//僵尸是否捕获
};
struct Zombie zms[10];
IMAGE imgZM[22];
IMAGE imgZMDead[20];
IMAGE imgZMEat[21];
IMAGE imgZmStand[11];

struct bullet
{
	int x, y;
	int row;
	bool used;
	int speed;
	bool blast;	//是否发射爆炸
	int frameIndex;	//帧序号
};
struct bullet bullets[64];
IMAGE imgButtletNormal;
IMAGE imgBullBlast[4];

int curShovel;

int bulletCount = sizeof(bullets) / sizeof(bullets[0]);

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
	//加载游戏背景
	loadimage(&imgBg, "res/bg.jpg");
	loadimage(&imgBar, "res/bar5.png");
	loadimage(&ShovelSlot, "res/shovelSlot.png");
	loadimage(&Shovel, "res/Screen/shovel.png");
	memset(imgPlant, 0, sizeof(imgPlant));	//把内存连续设置为一个值
	memset(map, 0, sizeof(map));
	//初始化植物卡牌以及初始化植物
	char name[64];
	for (int i = 0; i < plantCount; i++)
	{
		sprintf_s(name, sizeof(name), "res/Cards/card_%d.png", i + 1);
		loadimage(&imgPlantCards[i], name);
		for (int j = 0; j < 20; j++)
		{
			sprintf_s(name, sizeof(name), "res/zhiwu/%d/%d.png", i, j + 1);
			//先判断这个文件是否存在
			if (fileExist(name))
			{
				imgPlant[i][j] = new IMAGE;	//加载内存
				loadimage(imgPlant[i][j], name);
			}
			else 
			{
				break;
			}
		}

	}

	curPlant = 0;
	sunshine = 50;

	memset(balls, 0, sizeof(balls));	//赋值为0
	for (int i = 0; i < 29; i++)
	{
		sprintf_s(name, sizeof(name), "res/sunshine/%d.png", i + 1);
		loadimage(&imgSunshineBall[i], name);
	}

	//初始化僵尸数据
	memset(zms, 0, sizeof(zms));
	for (int i = 0; i < 22; i++)
	{
		sprintf_s(name, sizeof(name), "res/zm/%d.png", i + 1);
		loadimage(&imgZM[i],name);

	}
	//初始化僵尸死亡动画
	for (int i = 0; i < 20; i++)
	{
		sprintf_s(name, sizeof(name), "res/zm_dead/%d.png", i + 1);
		loadimage(&imgZMDead[i], name);
	}
	//初始化僵尸吃植物动画
	for (int i = 0; i < 21; i++)
	{
		sprintf_s(name, sizeof(name), "res/zm_eat/%d.png", i + 1);
		loadimage(&imgZMEat[i], name);
	}
	//初始化僵尸站立动画
	for (int i = 0; i < 11; i++)
	{
		sprintf_s(name, sizeof(name), "res/zm_stand/%d.png", i + 1);
		loadimage(&imgZmStand[i], name);
	}

	//初始化豌豆子弹
	loadimage(&imgButtletNormal, "res/bullets/bullet_normal.png");
	memset(bullets, 0, sizeof(bullets));
	//初始化豌豆子弹的帧图片数组
	loadimage(&imgBullBlast[3], "res/bullets/bullet_blast.png");
	for (int i = 0; i < 3; i++)
	{
		float k = (i + 1) * 0.2;
		loadimage(&imgBullBlast[i], "res/bullets/bullet_blast.png",
			imgBullBlast[3].getwidth() * k,
			imgBullBlast[3].getheight() * k, true);
	}

	//配置随机种子
	srand(time(NULL));

	//创建游戏图形窗口
	initgraph(SCREEN_WIDHT, SCREEN_HEIGHT);

	//设置字体
	LOGFONT F;
	gettextstyle(&F);	//获取当前字体
	F.lfHeight = 30;
	F.lfWeight = 15;
	strcpy(F.lfFaceName, "Segoe UI Black");
	F.lfQuality = ANTIALIASED_QUALITY;	//文本抗锯齿
	settextstyle(&F);
	setbkmode(TRANSPARENT);	//文本设置透明
	setcolor(BLACK);		//文本颜色

	killCount = 0;
	zmCount = 0;
	gameStatus = GOING;
}

void drawZM()
{
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < zmCount; i++)
	{
		if (zms[i].used)
		{
			IMAGE* img = NULL;
			if (zms[i].dead)
			{
				img = imgZMDead;
			}
			else if (zms[i].eating)
			{
				img = imgZMEat;
			}
			else
			{
				img = imgZM;
			}
			img += zms[i].frameIndex;	//指针的加法，每次执行一次加法，表示往前走若干个数据

			putimagePNG(zms[i].x, zms[i].y - img->getheight(), img);
		}
	}
}

void drawSunshines()
{
	//渲染阳光球旋转
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++)
	{
		if (balls[i].used)
		{
			IMAGE* img = &imgSunshineBall[balls[i].frameIndex];
			putimagePNG(balls[i].pCur.x, balls[i].pCur.y, img);
		}
	}

	char scoreText[8];
	sprintf_s(scoreText, sizeof(scoreText), "%d", sunshine);
	outtextxy(126, 67, scoreText);	//输出文本
}

void drawCards()
{
	for (int i = 0; i < plantCount; i++)
	{
		int x = 188 + i * 65;
		int y = 6;
		putimagePNG(x, y, &imgPlantCards[i]);
	}
}

void drawPlantAndShovel()
{
	//渲染植物
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type > 0)
			{
				//int x = 256 + j * 81;
				//int y = 179 + i * 102;
				int plantType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				putimagePNG(map[i][j].x, map[i][j].y, imgPlant[plantType][index]);
			}
		}
	}

	//渲染 拖动过程中的植物
	if (curPlant > 0)
	{
		IMAGE* img = imgPlant[curPlant - 1][0];
		putimagePNG(curX - img->getwidth() / 2, curY - img->getheight() / 2, imgPlant[curPlant - 1][0]);
	}

	//铲子
	if (curShovel == 1)
	{
		IMAGE* imgshovel = &Shovel;
		putimagePNG(curX - imgshovel->getwidth() / 2, curY - imgshovel->getheight() / 2, &Shovel);
	}
}

void drawBullets()
{
	//渲染子弹
	for (int i = 0; i < bulletCount; i++)
	{
		if (bullets[i].used)
		{
			if (bullets[i].blast)
			{
				IMAGE* img = &imgBullBlast[bullets[i].frameIndex];
				putimagePNG(bullets[i].x, bullets[i].y, img);
			}
			else
			{
				putimagePNG(bullets[i].x, bullets[i].y, &imgButtletNormal);
			}
		}
	}
}

void updataWindow()
{
	BeginBatchDraw();
	putimage(-112, 0, &imgBg);
	putimagePNG(100, 0, &imgBar);
	putimagePNG(779, 9, &ShovelSlot);
	putimagePNG(780, 9, &Shovel);

	mciSendString("play res/bg.MP3", 0, 0, 0);
	
	drawCards();
	
	drawPlantAndShovel();

	drawSunshines();

	drawZM();

	drawBullets();

	EndBatchDraw();
}

void collectSunshine(ExMessage* msg)
{
	int count = sizeof(balls) / sizeof(balls[0]);
	int wid = imgSunshineBall[0].getwidth();
	int hei = imgSunshineBall[0].getheight();
	for (int i = 0; i < count; i++)
	{
		if (balls[i].used)
		{
			//int x = balls[i].x;
			//int y = balls[i].y;
			int x = balls[i].pCur.x;
			int y = balls[i].pCur.y;
			if (msg->x > x && msg->x<x + wid && msg->y>y && msg->y < y + hei)
			{
				//balls[i].used = false;
				balls[i].status = SUNSHINE_COLLECT;
				//sunshine += 50;
				//mciSendString("play res/sunshine.mp3", 0, 0, 0);
				PlaySound("res/sunshine.wav", NULL, SND_FILENAME | SND_ASYNC);
				//设置阳光球的偏移量
				//float destY = 0;
				//float destX = 262;
				//float angle = atan((balls[i].y - destY) / (balls[i].x - destX));
				//balls[i].xoff = 4 * cos(angle);
				//balls[i].yoff = 4 * sin(angle);

				balls[i].p1 = balls[i].pCur;
				balls[i].p4 = vector2(112, 0);
				balls[i].t = 0;
				float distance = dis(balls[i].p1 - balls[i].p4);	//c++的运算符重载
				float off = 12;
				balls[i].speed = 1.0 / (distance / off);	//贝塞尔曲线计算速度，通过累加速度累加到1意味着到达终点坐标
				break;
			}
		}
	}
}

void delallPlant(ExMessage* msg)
{
	int row = (msg->y - 179) / 102;
	int col = (msg->x - 144) / 81;
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	if (curShovel == 1)
	{
		map[row][col].type = 0;
		map[row][col].blood = 0;
		for (int i = 0; i < zmCount; i++)
		{
			if (map[row][col].catched && zms[i].catching)
			{
				zms[i].eating = false;
				zms[i].frameIndex = 0;
				zms[i].speed = 2;
				zms[i].catching = false;
			}
		}
	}
}

void userClick()
{
	ExMessage msg;
	static int status = 0;
	if (peekmessage(&msg))		//判断是否有消息
	{
		if (msg.message == WM_LBUTTONDOWN)	//鼠标左键按下
		{
			if (msg.x > 188 && msg.x < 188 + 65 * plantCount && msg.y < 96)
			{
				int index = (msg.x - 188) / 65;
				printf("%d\n", index);
				status = 1;
				curPlant = index + 1;
			}
			else if (msg.x < 780 + 70 && msg.x>780 && msg.y > 9 && msg.y < 9 + 74)
			{
				status = 1;
				curShovel = 1;
			}
			else 
			{
				collectSunshine(&msg);
			}
		}
		else if (msg.message == WM_MOUSEMOVE && status == 1)	//鼠标拖动
		{
			curX = msg.x;
			curY = msg.y;
		}
		else if (msg.message == WM_LBUTTONUP)
		{
			if (msg.x > 144 && msg.y > 179 && msg.y < 489)
			{
				int row = (msg.y - 179) / 102;
				int col = (msg.x - 144) / 81;
				if (map[row][col].type == 0)
				{
					if (curPlant == Pea + 1 && sunshine >= 100)
					{
						map[row][col].type = curPlant;
						map[row][col].blood = 20;
						map[row][col].frameIndex = 0;
						map[row][col].shootTimer = 0;
						map[row][col].x = 144 + col * 81;
						map[row][col].y = 179 + row * 102 + 14;
						sunshine -= 100;
					}
					if (curPlant == sunFlower + 1 && sunshine >= 50)
					{
						map[row][col].type = curPlant;
						map[row][col].blood = 10;
						map[row][col].frameIndex = 0;
						map[row][col].shootTimer = 0;
						map[row][col].x = 144 + col * 81;
						map[row][col].y = 179 + row * 102 + 14;
						sunshine -= 50;
					}
					if (curPlant == Nut + 1 && sunshine >= 50)
					{
						map[row][col].type = curPlant;
						map[row][col].blood = 200;
						map[row][col].frameIndex = 0;
						map[row][col].shootTimer = 0;
						map[row][col].x = 144 + col * 81;
						map[row][col].y = 179 + row * 102 + 14;
						sunshine -= 50;
					}
				}

			}
			delallPlant(&msg);

			curPlant = 0;
			curShovel = 0;
			status = 0;
		}
	}
}

void createSunshine()
{
	static int count = 0;
	static int fre = 200;
	count++;
	if (count >= fre)
	{
		fre = 100 + rand() % 100;
		count = 0;

		//从阳光池中取一个可以使用的阳光
		int ballMax = sizeof(balls) / sizeof(balls[0]);		//获取当前数组一共有多少个数字
		int i = 0;
		for (i = 0; i < ballMax && balls[i].used; i++);
		if (i >= ballMax) return;

		balls[i].used = true;
		balls[i].frameIndex = 0;
		//balls[i].x = 260 + rand() % (900 - 260);
		//balls[i].y = 60;
		//balls[i].destY = 200 + (rand() % 4) * 90;
		balls[i].timer = 0;

		balls[i].status = SUNSHINE_DOWN;
		balls[i].t = 0;
		balls[i].p1 = vector2(148 + rand() % (900 - 148), 60);
		balls[i].p4 = vector2(balls[i].p1.x, 200 + (rand() % 4) * 90);
		int off = 6;
		float distance = balls[i].p4.y - balls[i].p1.y;
		balls[i].speed = 1.0 / (distance / off);
	}

	//向日葵生产阳光
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if (map[i][j].type == sunFlower + 1)
			{
				map[i][j].timer++;
				if (map[i][j].timer > 200)
				{
					map[i][j].timer = 0;
					int k;
					for (k = 0; k < ballMax && balls[k].used; k++);
					if (k >= ballMax)
					{
						return;
					}
					balls[k].used = true;
					balls[k].p1 = vector2(map[i][j].x, map[i][j].y); 
					int w = (100 + rand() % 50) * (rand() % 2 ? 1 : -1);
					balls[k].p4 = vector2(map[i][j].x+w,
						map[i][j].y+imgPlant[sunFlower][0]->getheight()-
						imgSunshineBall[0].getheight());	//向日葵产生阳光球的终点坐标
					balls[k].p2 = vector2(balls[k].p1.x + w * 0.3, balls[k].p1.y - 100);
					balls[k].p3 = vector2(balls[k].p1.x + w * 0.7, balls[k].p1.y - 100);
					balls[k].status = SUNSHINE_PRODUCE;
					balls[k].speed = 0.05;
					balls[k].t = 0;
				}
			}
		}
	}
}

void updataSunshine()
{
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++)
	{
		if (balls[i].used)
		{
			balls[i].frameIndex = (balls[i].frameIndex + 1) % 29;
			if (balls[i].status == SUNSHINE_DOWN)
			{
				struct sunshineBall* sunball = &balls[i];
				sunball->t += sunball->speed;
				sunball->pCur = sunball->p1 + sunball->t * (sunball->p4 - sunball->p1);
				if (sunball->t >= 1)
				{
					sunball->status = SUNSHINE_GROUND;
					sunball->timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_GROUND)
			{
				balls[i].timer++;
				if (balls[i].timer > 200)
				{
					balls[i].used = false;
					balls[i].timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_COLLECT)
			{
				struct sunshineBall* sunball = &balls[i];
				sunball->t += sunball->speed;
				sunball->pCur = sunball->p1 + sunball->t * (sunball->p4 - sunball->p1);
				if (sunball->t > 1)
				{
					sunball->used = false;
					sunshine += 50;
				}
			}
			else if (balls[i].status == SUNSHINE_PRODUCE)
			{
				struct sunshineBall* sunball = &balls[i];
				sunball->t += sunball->speed;
				sunball->pCur = calcBezierPoint(sunball->t, sunball->p1, sunball->p2, sunball->p3, sunball->p4);
				if (sunball->t > 1)
				{
					sunball->status = SUNSHINE_GROUND;
					sunball->timer = 0;
				}
			}
		}
	}
}

void createZM()
{
	if (zmCount >= ZM_MAX)
	{
		return;
	}
	static int zmFre = 400;
	static int count = 0;
	count++;
	if (count > zmFre)
	{
		count = 0;
		zmFre = rand() % 200 + 200;

		int i;
		int zmMax = sizeof(zms) / sizeof(zms[0]);
		for (i = 0; i < zmMax && zms[i].used; i++);
		if (i < zmMax)
		{
			memset(&zms[i], 0, sizeof(zms[i]));
			zms[i].used = true;
			zms[i].x = SCREEN_WIDHT;
			zms[i].row = rand() % 3;
			zms[i].y = 172 + (1 + zms[i].row) * 100;
			zms[i].speed = 2;
			zms[i].blood = 100;
			zms[i].eating = false;
			zms[i].dead = false;
			zmCount++;
		}
	}
}

void updataZM()
{
	static int count = 0;
	int zmMax = sizeof(zms) / sizeof(zms[0]);
	count++;
	if (count > 4)
	{
		count = 0;
		//更新僵尸的位置
		for (int i = 0; i < zmMax; i++)
		{
			if (zms[i].used)
			{
				zms[i].x -= zms[i].speed;
				if (zms[i].x < 56)
				{
					gameStatus = FAIL;
				}
			}
		}
	}

	static int count2 = 0;
	count2++;
	if (count2 > 2)
	{
		count2 = 0;
		for (int i = 0; i < zmMax; i++)
		{
			if (zms[i].used)
			{
				if (zms[i].dead)
				{
					zms[i].frameIndex++;
					if (zms[i].frameIndex >= 20)
					{
						zms[i].used = false;
						killCount++;
						if (killCount == ZM_MAX)
						{
							gameStatus = WIN;
						}
					}
				}
				else if (zms[i].eating)
				{
					zms[i].frameIndex = (zms[i].frameIndex + 1) % 21;
				}
				else
				{
					zms[i].frameIndex = (zms[i].frameIndex + 1) % 22;
				}
			}
		}
	}
}



void shoot()
{
	int lines[3] = { 0 };
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	int dangerX = SCREEN_WIDHT - imgZM[0].getwidth() / 2;
	for (int i = 0; i < zmCount; i++)
	{
		if (zms[i].used && zms[i].x < dangerX)
		{
			lines[zms[i].row] = 1;
		}
	}

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			if ((map[i][j].type == Pea + 1) && lines[i])
			{
				//static int count = 0;
				//count++;					这里之前采用此类计时，但是会出现三行共用一个计时器，导致有些行豌豆射不出子弹
				map[i][j].shootTimer++;
				if(map[i][j].shootTimer> 40)
				{
					map[i][j].shootTimer = 0;
					int k;
					for (k = 0; k < bulletCount && bullets[k].used; k++);
					if (k < bulletCount)
					{
						bullets[k].used = true;
						bullets[k].row = i;
						bullets[k].speed = 5;

						bullets[k].blast = false;
						bullets[k].frameIndex = 0;

						int plant_X = 144 + j * 81;
						int plant_Y = 179 + i * 102;
						bullets[k].x = plant_X + imgPlant[map[i][j].type - 1][0]->getwidth() - 10;
						bullets[k].y = plant_Y + 20;
					}
				}
			}
		}
	}
}

void updataBullets()
{
	for (int i = 0; i < bulletCount; i++)
	{
		if (bullets[i].used)
		{
			bullets[i].x += bullets[i].speed;
			if (bullets[i].x > SCREEN_WIDHT)
			{
				bullets[i].used = false;
			}

			//待完善子弹的碰撞检测
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

void checkBulletFightZM()	//子弹对僵尸的碰撞检测
{
	int zCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < bulletCount; i++)
	{
		if (bullets[i].used == false || bullets[i].blast)
		{
			continue;
		}
		for (int k = 0; k < zCount; k++)
		{
			if (zms[k].used == false)
			{
				continue;
			}
			int x1 = zms[k].x + 80;
			int x2 = zms[k].x + 110;
			if (zms[k].dead == false && bullets[i].row == zms[k].row && bullets[i].x > x1 && bullets[i].x < x2)
			{
				zms[k].blood -= 10;
				bullets[i].blast = true;
				bullets[i].speed = 0;

				if (zms[k].blood <= 0)
				{
					zms[k].dead = true;
					zms[k].speed = 0;
					zms[k].frameIndex = 0;
				}
				break;
			}
		}
	}
}

void checkZMFightPlant()	//僵尸对子弹的碰撞检测
{
	int zCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < zCount; i++)
	{
		if (zms[i].dead)
		{
			continue;
		}
		int row = zms[i].row;
		for (int k = 0; k < 9; k++)
		{
			
			if (map[row][k].type == 0)
			{
				continue;
			}
			int plantX = 144 + k * 81;
			int plant_XOFF_1 = plantX + 10;
			int plant_XOFF_2 = plantX + 60;
			int zm_X = zms[i].x + 80;
			if (zm_X > plant_XOFF_1 && zm_X < plant_XOFF_2)
			{
				if (zms[i].catching && map[row][k].catched)
				{
					if (zms[i].frameIndex > 10)
					{
						map[row][k].blood -= 1;
						if (map[row][k].blood == 0)
						{
							map[row][k].type = 0;
							map[row][k].catched = false;
							zms[i].eating = false;
							zms[i].frameIndex = 0;
							zms[i].speed = 2;
							zms[i].catching = false;
						}
					}
				}
				else
				{
					map[row][k].catched = true;
					zms[i].eating = true;
					zms[i].speed = 0;
					zms[i].frameIndex = 0;
					zms[i].catching = true;
				}
			}
		}
	}
}

void collisionCheck()	//碰撞检测
{
	checkBulletFightZM();
	checkZMFightPlant();
}

void updataPlant()
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
				int plantType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				if (imgPlant[plantType][index] == NULL)
				{
					map[i][j].frameIndex = 0;
				}
			}
		}
	}
}

void updataGame()
{
	updataPlant();

	createSunshine();
	updataSunshine();//更新阳光的状态

	createZM();
	updataZM();//更新僵尸的状态

	shoot();	//发射豌豆子弹
	updataBullets();
	collisionCheck();	//实现豌豆子弹和僵尸的碰撞检测
}

void startUI()
{
	IMAGE imgBg, imgMenu1, imgMenu2;
	loadimage(&imgBg, "res/menu.png");
	loadimage(&imgMenu1, "res/menu1.png");
	loadimage(&imgMenu2, "res/menu2.png");
	int flag = 0;

	while (1)
	{
		BeginBatchDraw();
		putimage(0, 0, &imgBg);
		putimagePNG(474, 75, flag ? &imgMenu2 : &imgMenu1);
		

		ExMessage msg;
		if (peekmessage(&msg))
		{
			if (msg.message == WM_LBUTTONDOWN &&
				msg.x > 474 && msg.x < 474 + 310 &&
				msg.y>75 && msg.y < 75 + 145)
			{
				flag = 1;

			}
			else if (msg.message == WM_LBUTTONUP && flag)
			{
				EndBatchDraw();
				break;
			}
		}
		EndBatchDraw();
	}
}

void viewScence()
{
	int xMin = SCREEN_WIDHT - imgBg.getwidth();	//900-1400=-500
	vector2 points[9] = { {550,80},{530,160},{630,170},{530,200},
		{515,270},{565,370},{605,340},{705,280},{690,340} };
	int index[9];
	for (int i = 0; i < 9; i++)
	{
		index[i] = rand() % 11;
	}
	int count = 0;
	for (int x = 0; x >= xMin; x -= 2)
	{
		BeginBatchDraw();
		putimage(x, 0, &imgBg);
		count++;
		for (int k = 0; k < 9; k++)
		{
			putimagePNG(points[k].x - xMin + x, points[k].y, &imgZmStand[index[k]]);
			if (count >= 10)
			{
				index[k] = (index[k] + 1) % 11;
			}
		}
		if (count >= 10)count = 0;
		EndBatchDraw();
		Sleep(5);
	}
	//停留2秒左右
	for (int i = 0; i < 50; i++)
	{
		BeginBatchDraw();
		putimage(xMin, 0, &imgBg);
		for (int k = 0; k < 9; k++)
		{
			putimagePNG(points[k].x, points[k].y, &imgZmStand[index[k]]);
			index[k] = (index[k] + 1) % 11;
		}
		EndBatchDraw();
		Sleep(30);
	}

	for (int x = xMin; x <= -112; x += 2)
	{
		BeginBatchDraw();
		putimage(x, 0, &imgBg);

		count++;
		for (int k = 0; k < 9; k++)
		{
			putimagePNG(points[k].x - xMin + x, points[k].y, &imgZmStand[index[k]]);
			if (count >= 10)
			{
				index[k] = (index[k] + 1) % 11;
			}
		}
		if (count >= 10) count = 0;
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
		putimage(-112, 0, &imgBg);
		putimagePNG(100, y, &imgBar);
		putimagePNG(779, y+9, &ShovelSlot);
		putimagePNG(780, y+9, &Shovel);

		for (int i = 0; i < plantCount; i++)
		{
			int x = 188 + i * 65;
			y += 6;
			putimagePNG(x, y, &imgPlantCards[i]);
		}
		EndBatchDraw();
		Sleep(20);
	}
}

bool checkOver()
{
	bool ret = false;
	if (gameStatus == WIN)
	{
		Sleep(2000);
		loadimage(0, "res/win2.png");
		ret = true;
	}
	else if (gameStatus == FAIL)
	{
		Sleep(2000);
		loadimage(0, "res/fail2.png");
		ret = true;
	}
	return ret;
}

int main(void)
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
		timer += getDelay();
		if (timer > 20)
		{
			flag = true;
			timer = 0;
		}
		if (flag)
		{
			flag = false;
			updataGame(); 
			if (checkOver())break;		//表示游戏是否结束
		}
		updataWindow();
	}
	system("pause");
	return 0;
}
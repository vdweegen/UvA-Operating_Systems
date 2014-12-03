static int initChopsticks();
static int getSeat();
static int pickup_chopstick(int mySeat, int side);
static int drop_chopsticks(int mySeat);
static void * diner(void * args);
static void * my_finale();
static int aggressiveRight(int left, int right, int mySeat);
static int aggressiveLeft(int left, int right, int mySeat);
static int oppertunistic(int left, int right, int mySeat);
static int shy(int left, int right, int mySeat);
int count_chars(const char* string, char ch);

int yield();

static volatile int payNow = 0;    // Use to end the feast

typedef int strategyFun(int stick1, int stick2);

typedef struct DINERARGS
{
    strategyFun * strategy;
}
dinerArgs;
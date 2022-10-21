#pragma once

class CycCnt
{
private:
	static unsigned int SCLK;
	static unsigned int SCLK_1us;

	static constexpr volatile unsigned int *DWT_CYCCNT() {
		void *p = reinterpret_cast<void *>(0xE0001004);
		return reinterpret_cast<volatile unsigned int *>(p);
	}
	CycCnt();
public:

	// 次のトレースおよびデバッグブロックの使用を許可するには、このビットを1にセットする必要があります。
	// データウォッチポイントおよびトレース(DWT)ユニット
	// 計装トレース マクロセル(ITM)
	// エンベデッドトレース マクロセル(ETM)
	// トレースポート インタフェースユニット(TPIU)。
	static const int TRCENA = 0x01000000;
	static const int CYCCNTENA = 1; // CYCCNT enable
	static void Init() {
		//DWT_CYCCNT  = (volatile unsigned int *)0xE0001004;
		volatile unsigned int *SCB_DEMCR   = (volatile unsigned int *)0xE000EDFC; //address of the register
		volatile unsigned int *DWT_CONTROL = (volatile unsigned int *)0xE0001000; //address of the register
		*SCB_DEMCR = *SCB_DEMCR | TRCENA; // TRCENA = 1
		*DWT_CYCCNT() = 0; // reset the counter
		*DWT_CONTROL = *DWT_CONTROL | CYCCNTENA ; // enable the counter (CYCCNTENA = 1)
		SCLK = 72000000;
		SCLK_1us = SCLK / 1000000;
	}

	static int GetSCLK() {
		return SCLK;
	}

	static int GetSCLK_1us() {
		return SCLK_1us;
	}

	static uint32_t getDwtCyccnt() {
		return *DWT_CYCCNT();
	}

	//tickに0を指定したとしても10クロック前後は消費する
	static void TimingDelay(unsigned int tick)
	{
		unsigned int start, current;
		start = *DWT_CYCCNT();
		do {
			current = *DWT_CYCCNT();
		} while((current - start) < tick);
	}

	static void TimingDelay_us(unsigned int tick)
	{
		TimingDelay(tick * SCLK_1us);
	}

	//HAL_Delayで十分な気がする
	static void TimingDelay_ms(unsigned int tick)
	{
		TimingDelay(tick * SCLK_1us * 1000);
	}

	//分解能は180MHz動作で22ns程度。計算している間に時間が経過するので事前に計算しておいて
	//waitが必要な時にTimingDelayを呼ぶ
	static uint32_t tick_ns(unsigned int ns)
	{
		return ( ns * SCLK_1us + 999) / 1000;
	}
};

struct Timeup {
	uint32_t current;
	uint32_t wait_time;
	Timeup(uint32_t us) : current(CycCnt::getDwtCyccnt()),
			wait_time(us * CycCnt::GetSCLK_1us()){
	}
	bool isTimeup() {
		return CycCnt::getDwtCyccnt() - current > wait_time;
	}
};


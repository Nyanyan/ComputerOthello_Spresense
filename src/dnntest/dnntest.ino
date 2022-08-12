#include <DNNRT.h>
#include <Flash.h>
#include <File.h>

DNNRT dnnrt;
DNNVariable input0(64);
DNNVariable input1(64);

#define FILLED 1.0

void setup() {
  Serial.begin(115200);

  File nnbfile = Flash.open("model.nnb", FILE_READ);
  if (!nnbfile) {
    Serial.println("model.nnb is not found");
    while(1);
  }
  int ret = dnnrt.begin(nnbfile);  // DNNRTを初期化
  if (ret < 0) {
    Serial.println("DNNRT begin fail: " + String(ret));
    while(1);
  }

  float* buf0 = input0.data();  // DNNRT変数用内部バッファ取得
  for (int n = 0; n < 64; ++n)
    buf0[n] = 0.0;
  buf0[27] = FILLED;

  float* buf1 = input1.data();  // DNNRT変数用内部バッファ取得
  for (int n = 0; n < 64; ++n)
    buf1[n] = 0.0;
  buf1[28] = FILLED;
  buf1[35] = FILLED;
  buf1[36] = FILLED;
  buf1[37] = FILLED;

  dnnrt.inputVariable(input0, 0);  // 入力データを設定
  dnnrt.inputVariable(input1, 1);  // 入力データを設定
  dnnrt.forward();  // 推論を実行
  DNNVariable output0 = dnnrt.outputVariable(0);  // 出力を取得
  DNNVariable output1 = dnnrt.outputVariable(1);  // 出力を取得

  for (int i = 0; i < 64; ++i){
    Serial.print(i);
    Serial.print("\t");
    Serial.println(output0[i]);
  }

  // 確からしさがもっとも高いインデックス値を取得
  int index = output0.maxIndex();
  Serial.println("Result: " + String(index));
  Serial.println("Probability: " + String(output0[index]));
  Serial.println("Value: " + String(output1[0]));
}

void loop() {}

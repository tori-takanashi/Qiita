#include "FDMSolverForHeatEq.h"
#include "SettingAnalyticProperties.h"
#include "VtkIndexUtility.h"

#include <vtkXMLUnstructuredGridReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDataSetWriter.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkCellData.h>
#include <vtkCell.h>

#include <vtkSmartPointer.h>

using namespace std;

FDMSolverForHeatEq::FDMSolverForHeatEq()
{
}

FDMSolverForHeatEq::~FDMSolverForHeatEq()
{
}

// 設計適当ですが許して
// 第一引数：解析場の情報を保存，第二引数：解析場中の障害物表面の場所，第三引数：正の値で終了ステップを入力
void FDMSolverForHeatEq::SolveHeatEquationForAllStep(vtkUnstructuredGrid *out_field, const int &in_endStep)
{
  vtkIdType NumCellsInfield = out_field->GetNumberOfCells();
  //const double alpha = 0.01;
  SetParameters(out_field, alpha); // 解析パラメータ
  auto initResults = vtkSmartPointer<vtkDoubleArray>::New();
  initResults->SetName("Results");
  initResults->SetNumberOfComponents(1);
  for (auto vId = 0; vId < NumCellsInfield; vId++)
    initResults->InsertNextTuple1(0.0);

  out_field->GetCellData()->AddArray(initResults);

  // 計算
  int step;
  for (step = 0; step < in_endStep; step++)
  {
    if (!SolveHeatEquationForOneStep(out_field))
      break;
  }
  cout << step << "で終了" << endl;
}

// なんかすごいことしてるかもしれないけど許してくれ
bool FDMSolverForHeatEq::SolveHeatEquationForOneStep(vtkUnstructuredGrid *out_field)
{
  // 一つ前の結果を取り出す
  auto beforeResults = vtkSmartPointer<vtkDoubleArray>::New();
  beforeResults->DeepCopy(vtkDoubleArray::SafeDownCast(out_field->GetCellData()->GetArray("Results")));
  if (!beforeResults) return false;

  // 解析属性
  auto properties = out_field->GetCellData()->GetArray("AnalyticProoerty");
  if (!properties)
  {
    cout << "解析用の属性がボクセルに設定されていません。" << endl;
    return false;
  }

  // 境界条件
  SetBoundryCondition(beforeResults, properties);

  // 次の計算結果
  auto nextResults = vtkSmartPointer<vtkDoubleArray>::New();
  nextResults->DeepCopy(beforeResults);

  // 全ボクセルを参照
  for (vtkIdType vId = 0; vId < out_field->GetNumberOfCells(); vId++)
  {
    const vtkIdType *xyzId = VtkIndexUtility::ConvertVtkCellIdToXYZ(vId, cellDims);

    // 解析属性がCalculation以外なら飛ばす。
    if (properties->GetTuple1(vId) != Analysis)
      continue;

    const double beforeRes = beforeResults->GetTuple1(vId);

    // TODO：変数名変更
    const vtkIdType *peripheralXYZs = VtkIndexUtility::GetPeripheralXYZs(xyzId[0], xyzId[1], xyzId[2], cellDims);

    const double deltas[3] = { this->deltas[0], this->deltas[1], this->deltas[2] };

    // 急にラムダ使ってすまない
    auto getRes = [&](const vtkIdType &cellId) -> const double
    { return beforeResults->GetTuple1(cellId); };

    // 計算☆
    const double nextRes = beforeRes + this->dt * this->alpha *
      (
      (getRes(peripheralXYZs[0]) - 2.0 * beforeRes + getRes(peripheralXYZs[1])) / (deltas[0] * deltas[0]) +
        (getRes(peripheralXYZs[2]) - 2.0 * beforeRes + getRes(peripheralXYZs[3])) / (deltas[1] * deltas[1]) +
        (getRes(peripheralXYZs[4]) - 2.0 * beforeRes + getRes(peripheralXYZs[5])) / (deltas[2] * deltas[2])
        );

    nextResults->SetTuple1(vId, nextRes);
  }

  out_field->GetCellData()->AddArray(nextResults);
  return true;
}

void FDMSolverForHeatEq::SetParameters(vtkUnstructuredGrid *out_field, const double &in_alpha)
{
  // 解析パラメータ ----
  double *fieldBounds = out_field->GetBounds();
  // out_field の各方向の大きさ
  this->lengths[0] = fieldBounds[1] - fieldBounds[0];
  this->lengths[1] = fieldBounds[3] - fieldBounds[2];
  this->lengths[2] = fieldBounds[5] - fieldBounds[4];
  // (等間隔格子の場合)どれでもいいのでセルをとってきてその大きさを調べる
  vtkCell *voxel = out_field->GetCell(0);
  const double voxelSizes[3] =
  {
    voxel->GetBounds()[1] - voxel->GetBounds()[0],
    voxel->GetBounds()[3] - voxel->GetBounds()[2],
    voxel->GetBounds()[5] - voxel->GetBounds()[4]
  };
  // XYZ各方向のセル数を調べる
  this->cellDims[0] = round(this->lengths[0] / voxelSizes[0]);
  this->cellDims[1] = round(this->lengths[1] / voxelSizes[1]);
  this->cellDims[2] = round(this->lengths[2] / voxelSizes[2]);

  // 空間刻み幅
  this->deltas[0] = this->lengths[0] / this->cellDims[0];
  this->deltas[1] = this->lengths[1] / this->cellDims[1];
  this->deltas[2] = this->lengths[2] / this->cellDims[2];

  this->alpha = in_alpha;   // > 0

  // フォン・ノイマンの安定性解析に基づき時間刻み幅を設定
  this->dt = 0.5 / (in_alpha *
    (
      1.0 / (this->deltas[0] * this->deltas[0]) +
      1.0 / (this->deltas[1] * this->deltas[1]) +
      1.0 / (this->deltas[2] * this->deltas[2])
      ));
}

// 周辺のボクセルから平均値を求める
void FDMSolverForHeatEq::SetAveValueFromSurroundings(const vtkIdType in_voxelId, vtkDoubleArray *out_results, vtkDataArray *in_properties)
{
  const vtkIdType *xyzId = VtkIndexUtility::ConvertVtkCellIdToXYZ(in_voxelId, cellDims);

  // ボクセル周辺6個の平均をとる

  const vtkIdType *peripheralXYZs = VtkIndexUtility::GetPeripheralXYZs(xyzId[0], xyzId[1], xyzId[2], cellDims);

  int count = 0;
  double sumResult = 0.0;
  for (vtkIdType pXyzId = 0; pXyzId < 6; pXyzId++)
  {
    vtkIdType vId = peripheralXYZs[pXyzId];

    // 配列中の確保している要素以外は参照しない
    if (vId < 0 || out_results->GetNumberOfTuples() <= vId)
      continue;
    if (in_properties->GetTuple1(vId) != Analysis)
      continue;

    sumResult += out_results->GetValue(vId);
    count++;
  }

  out_results->SetValue(in_voxelId, sumResult / count);
}

void FDMSolverForHeatEq::SetBoundryCondition(vtkDoubleArray * in_results, vtkDataArray * in_properties)
{
  // 全ボクセル参照
  for (auto vId = 0; vId < in_properties->GetNumberOfTuples(); vId++)
  {
    vtkIdType propId = in_properties->GetTuple1(vId);

    // 3Dモデル表面なら、周囲の属性がCalculationのボクセルに定義されている計算値の平均値をセット
    if (propId == Surface3DModel)
      SetAveValueFromSurroundings(vId, in_results, in_properties);
    // 壁
    else if (propId == WallX)
      in_results->SetTuple1(vId, wallXYZBC[0]);
    else if (propId == WallY)
      in_results->SetTuple1(vId, wallXYZBC[1]);
    else if (propId == WallZ)
      in_results->SetTuple1(vId, wallXYZBC[2]);
  }

}


#include <iostream>

#include <vtkSelectEnclosedPoints.h>

#include <vtkGeometryFilter.h>
#include <vtkUnstructuredGrid.h>
#include <vtkCellCenters.h>
#include <vtkSphereSource.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkThreshold.h>
#include <vtkStructuredGrid.h>
#include <vtkAppendFilter.h>

#include <vtkDataSetReader.h>
#include <vtkDataSetWriter.h>

#include <vtkSmartPointer.h>

void GenerateBaseMeshFromAABB(vtkDataSet *out_baseMesh, vtkPolyData *in_polyData, const int in_cellDims[3]);

int main()
{
  // ① 閉曲面ポリゴンデータを作る ----
  auto reader = vtkSmartPointer<vtkDataSetReader>::New();
  reader->SetFileName("C:\\test\\ushi.vtk");
  //reader->SetFileName("C:\\test\\fillHole.vtk");
  reader->Update();

  // ソリッドをサーフェスに変える。元々サーフェスならなんも変わらない
  auto geometry = vtkSmartPointer<vtkGeometryFilter>::New();  
  geometry->SetInputData(reader->GetOutput());
  geometry->Update();

  vtkPolyData *closedPoly = geometry->GetOutput(); // polydataのポインタで形状を管理(やらなくてもいい)
  // ----

  // ② 基礎メッシュを作る ----
  auto baseMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();  // 基礎メッシュ
  const int cellDims[] = { 50, 50, 50 };
  GenerateBaseMeshFromAABB(baseMesh, closedPoly, cellDims);
  // ----

  // ③ 点群データを作る ----
  // 全ボクセル中心座標をボクセル数だけ作る
  auto cellCenters = vtkSmartPointer<vtkCellCenters>::New();
  cellCenters->SetInputData(baseMesh);
  cellCenters->Update();  // 全ボクセル中心座標を取得

  vtkPolyData *polyPoints = cellCenters->GetOutput(); // polydataのポインタで形状を管理(やらなくてもいい)
  // ----

  // ④ vtkSelectEnclosedPointsで内外判定 ----
  auto selectEnclosedPts = vtkSmartPointer<vtkSelectEnclosedPoints>::New();
  selectEnclosedPts->SetInputData(polyPoints);    // 点群セット
  selectEnclosedPts->SetSurfaceData(closedPoly);  // 閉曲面セット
  selectEnclosedPts->SetTolerance(1e-5);
  selectEnclosedPts->Update();
  // ----

  // ⑤ フィルタ適用、フィルタが持つ形状にぶら下がっている
  // "SelectedPoints"という配列を参照してbaseMeshのCellDataに与える
  vtkDataArray *isInsideOrOutside = selectEnclosedPts->GetOutput()->GetPointData()->GetArray("SelectedPoints");
  baseMesh->GetCellData()->AddArray(isInsideOrOutside); // PointDataで配列を取得した後、CellDataにぶらさげるので注意

  // 形状のCellDataにぶら下がっている"SelectedPoints"という配列から情報を抽出する
  auto threshold = vtkSmartPointer<vtkThreshold>::New();
  threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "SelectedPoints");
  threshold->SetInputData(baseMesh);
  threshold->ThresholdBetween(1, 1);  // (min, max) = (1, 1)
  threshold->Update();

  // ⑥ VTKレガシー形式で出力
  auto writer = vtkSmartPointer<vtkDataSetWriter>::New();
  writer->SetInputData(threshold->GetOutput());
  writer->SetFileName("C:\\test\\voxelMesh.vtk");
  writer->Update();
}

// 閉曲面のAABB(Axis-Aligned Bounding Box)から基礎メッシュを作成
// 第1引数に空の形状データを与えること
void GenerateBaseMeshFromAABB(vtkDataSet *out_baseMesh, vtkPolyData *in_polyData, const int in_cellDims[3])
{
  // 諸元
  auto bounds = in_polyData->GetBounds();
  const double meshPitches[3] =
  {
    (bounds[1] - bounds[0]) / in_cellDims[0],
    (bounds[3] - bounds[2]) / in_cellDims[1],
    (bounds[5] - bounds[4]) / in_cellDims[2]
  };
  const double mins[3] = { bounds[0], bounds[2], bounds[4] };

  // 等間隔格子点生成
  auto points = vtkSmartPointer<vtkPoints>::New();
  for (int zId = 0; zId < in_cellDims[2] + 1; zId++)
  {
    for (int yId = 0; yId < in_cellDims[1] + 1; yId++)
    {
      for (int xId = 0; xId < in_cellDims[0] + 1; xId++)
      {
        const double x = xId * meshPitches[0] + mins[0];
        const double y = yId * meshPitches[1] + mins[1];
        const double z = zId * meshPitches[2] + mins[2];
        points->InsertNextPoint(x, y, z);
      }
    }
  }

  // 閉曲面を包括するAABB内に構造格子を生成
  auto baseMesh = vtkSmartPointer<vtkStructuredGrid>::New();
  baseMesh->SetExtent(0, in_cellDims[0], 0, in_cellDims[1], 0, in_cellDims[2]);
  baseMesh->SetPoints(points);

  // 何かと使いにくいことがあるので、vtkStructuredGridをvtkUnstructuredGridに変換
  auto append = vtkSmartPointer<vtkAppendFilter>::New();
  append->AddInputData(baseMesh);
  append->Update();

  // 第1引数にコピー
  out_baseMesh->DeepCopy(append->GetOutput());
}
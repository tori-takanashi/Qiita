#include <iostream>

#include <vtkFillHolesFilter.h>

#include <vtkGeometryFilter.h>
#include <vtkPolyData.h>
#include <vtkCleanPolyData.h>
#include <vtkPolyDataNormals.h>

#include <vtkDataSetReader.h>
#include <vtkDataSetWriter.h>

#include <vtkSmartPointer.h>

int main()
{
  // ① 穴があるポリゴンデータを読み込む ----
  auto reader = vtkSmartPointer<vtkDataSetReader>::New();
  reader->SetFileName("C:\\test\\ushiThatCutLeg.vtk");
  reader->Update();

  // vtkUnstructuredGrid -> vtkPolyData
  auto geometry = vtkSmartPointer<vtkGeometryFilter>::New();
  geometry->SetInputData(reader->GetOutput());
  geometry->Update();

  vtkPolyData *closedPoly = geometry->GetOutput();
  // ----

  // ② 穴を埋める ----
  auto fillHole = vtkSmartPointer<vtkFillHolesFilter>::New();
  fillHole->SetInputData(closedPoly);
  fillHole->SetHoleSize(100.0); // 適当なサイズを入力してください
  fillHole->Update();
  // ----

  // ③ 重複節点を消す ----
  // 形状の大きさからトレランスを求める
  double tol = 0.0;
  double *bds = fillHole->GetOutput()->GetBounds();
  for (int id = 0; id < 3; id++)
    tol += abs(bds[id * 2 + 1] - bds[id * 2]);
  tol = tol / 3.0 * 0.001; // 形状の大きさの平均の0.1%

  auto clean = vtkSmartPointer<vtkCleanPolyData>::New();
  clean->SetInputData(fillHole->GetOutput());
  clean->SetTolerance(tol);
  clean->Update();
  // ----

  // ④ VTKレガシー形式で出力
  auto writer = vtkSmartPointer<vtkDataSetWriter>::New();
  writer->SetInputData(clean->GetOutput());
  writer->SetFileName("C:\\test\\fillHole.vtk");
  writer->Update();
}
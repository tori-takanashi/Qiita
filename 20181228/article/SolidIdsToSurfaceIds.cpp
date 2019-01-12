#include <iostream>

#include <vtkDataSetReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkIdTypeArray.h>
#include <vtkCellData.h>
#include <vtkGeometryFilter.h>
#include <vtkDataSetWriter.h>

void SetDSCellIds(vtkDataSet *inDS, const char *inArrayName);

int main(int, char *argv[])
{
  // vtk legacy format の形状(ソリッド)を読み込む
  vtkSmartPointer<vtkDataSetReader> reader =
    vtkSmartPointer<vtkDataSetReader>::New();
  reader->SetFileName("C:\\testVTK\\testUnstructuredGrid.vtk");
  reader->Update();

  vtkSmartPointer<vtkUnstructuredGrid> unsGrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  unsGrid->DeepCopy(reader->GetOutput());   // readerの持つ形状をコピー
  //dataSet->NewInstance();
  // セルの通し番号を登録
  SetDSCellIds(unsGrid, "OriginalCellIds");

  // ソリッド → サーフェス
  vtkSmartPointer<vtkGeometryFilter> geometry =
    vtkSmartPointer<vtkGeometryFilter>::New();
  geometry->SetInputData(unsGrid);
  geometry->Update();

  // vtk legacy format の形状(サーフェス)を書き込む
  vtkSmartPointer<vtkDataSetWriter> writer =
    vtkSmartPointer<vtkDataSetWriter>::New();
  writer->SetInputConnection(geometry->GetOutputPort());
  writer->SetFileName("C:\\testVTK\\testSurface.vtk");
  writer->Update();

  return 0;
}

void SetDSCellIds(vtkDataSet *inDS, const char *inArrayName)  // 汎用性を考慮して、引数は抽象クラスで受け取るようにする
{
  // 空のvtkDataArrayに形状データのセル通し番号を登録
  vtkSmartPointer<vtkIdTypeArray> idArray =
    vtkSmartPointer<vtkIdTypeArray>::New();
  idArray->SetName(inArrayName);
  idArray->SetNumberOfComponents(1); // 1次元の配列を定義

  for (int cId = 0; cId < inDS->GetNumberOfCells(); cId++)
  {
    idArray->InsertNextTuple1(cId);
  }

  // 形状データのCellDataにDataArrayを登録する
  inDS->GetCellData()->AddArray(idArray);
}

#include <iostream>

#include <vtkDataSetReader.h>
#include <vtkGeometryFilter.h>
#include <vtkDataSetWriter.h>

int main(int, char *argv[])
{
  // vtk legacy format の形状(ソリッド)を読み込む
  vtkSmartPointer<vtkDataSetReader> reader =
    vtkSmartPointer<vtkDataSetReader>::New();
  reader->SetFileName("C:\\testVTK\\testUnstructuredGrid.vtk");
  reader->Update();

  // ソリッド → サーフェス
  vtkSmartPointer<vtkGeometryFilter> geometry =
    vtkSmartPointer<vtkGeometryFilter>::New();
  geometry->SetInputConnection(reader->GetOutputPort());
  geometry->Update();

  // vtk legacy format の形状(サーフェス)を書き込む
  vtkSmartPointer<vtkDataSetWriter> writer =
    vtkSmartPointer<vtkDataSetWriter>::New();
  writer->SetInputConnection(geometry->GetOutputPort());
  writer->SetFileName("C:\\testVTK\\testSurface.vtk");
  writer->Update();

  return 0;
}

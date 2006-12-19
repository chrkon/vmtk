/*=========================================================================
                                                                                                                                    
Program:   VMTK
Module:    $RCSfile: vtkvmtkFDNEUTReader.cxx,v $
Language:  C++
Date:      $Date: 2006/04/06 16:47:47 $
Version:   $Revision: 1.8 $
                                                                                                                                    
  Copyright (c) Luca Antiga, David Steinman. All rights reserved.
  See LICENCE file for details.

  Portions of this code are covered under the VTK copyright.
  See VTKCopyright.txt or http://www.kitware.com/VTKCopyright.htm 
  for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
                                                                                                                                    
=========================================================================*/

#include "vtkvmtkFDNEUTReader.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkCellData.h"
#include "vtkCellArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkvmtkConstants.h"

vtkCxxRevisionMacro(vtkvmtkFDNEUTReader, "$Revision: 1.8 $");
vtkStandardNewMacro(vtkvmtkFDNEUTReader);

vtkvmtkFDNEUTReader::vtkvmtkFDNEUTReader()
{
  this->GhostNodes = 1;
  this->VolumeElementsOnly = 0;
  this->SingleCellDataEntityArrayName = NULL;
}

vtkvmtkFDNEUTReader::~vtkvmtkFDNEUTReader()
{
  if (this->SingleCellDataEntityArrayName)
    {
    delete[] this->SingleCellDataEntityArrayName;
    this->SingleCellDataEntityArrayName = NULL;
    }
}

int vtkvmtkFDNEUTReader::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
    {
    return 1;
    }

  if (!this->FileName)
    {
    vtkErrorMacro(<<"FileName not set.");
    return 1;
    }
        
  FILE* FDNEUTFile;
  FDNEUTFile = fopen(this->FileName,"r");

  if (!FDNEUTFile)
    {
    vtkErrorMacro(<<"Could not open file.");
    return 1;
    }

  char buffer[1024];
  do
    {
    fscanf(FDNEUTFile, "%s", buffer);
    while (fgetc(FDNEUTFile) != '\n');
    }
  while (strncmp(buffer,"NODAL",5)!=0);
  //  while (fgetc(FDNEUTFile) != '\n');

  int pointId;
  float point[3], doublePoint[3];

  vtkPoints* points = vtkPoints::New();

  while(fscanf(FDNEUTFile, "%d", &pointId))
    {
    fscanf(FDNEUTFile, "%f", &point[0]);
    fscanf(FDNEUTFile, "%f", &point[1]);
    fscanf(FDNEUTFile, "%f", &point[2]);
    while (fgetc(FDNEUTFile) != '\n');

    doublePoint[0] = point[0];
    doublePoint[1] = point[1];
    doublePoint[2] = point[2];

    points->InsertPoint(pointId-1,doublePoint);
    }

  int numberOfPoints = points->GetNumberOfPoints();

  int cellId, i;
  int nodesPerElement, geometry, fdneutType;

  vtkCellArray* gridCellArray = vtkCellArray::New();

  int entityCounter = 0;
  vtkUnsignedCharArray* singleEntityArray;
  singleEntityArray = vtkUnsignedCharArray::New();
  singleEntityArray->SetName(this->SingleCellDataEntityArrayName);
  output->GetCellData()->AddArray(singleEntityArray);
 
  vtkIntArray* typesArray = vtkIntArray::New();
  while (fscanf(FDNEUTFile, "%s", buffer)!=EOF)
    {
    while (strncmp(buffer,"NODES:",6)!=0)
      fscanf(FDNEUTFile, "%s", buffer);
    fscanf(FDNEUTFile, "%d", &nodesPerElement);
    while (strncmp(buffer,"GEOMETRY:",9)!=0)
      fscanf(FDNEUTFile, "%s", buffer);
    fscanf(FDNEUTFile, "%d", &geometry);
    while (strncmp(buffer,"TYPE:",5)!=0)
      fscanf(FDNEUTFile, "%s", buffer);
    fscanf(FDNEUTFile, "%d", &fdneutType);
                
    while (fgetc(FDNEUTFile) != '\n');
    fscanf(FDNEUTFile, "%s", buffer);
    fscanf(FDNEUTFile, "%s", buffer);

    char currentEntityName[256];
    fscanf(FDNEUTFile, "%s", currentEntityName);
    while (fgetc(FDNEUTFile) != '\n');

    int type;
    int pointBuffer;
    int numberOfCellPoints = 0;
#if 0
    vtkUnsignedCharArray* entityArray = NULL;
    entityArray = vtkUnsignedCharArray::New();
    entityArray->SetName(currentEntityName);
    output->GetCellData()->AddArray(entityArray);
#endif
    while(fscanf(FDNEUTFile, "%d", &cellId)>0)
      {
      int* cellPoints = NULL;
      bool invalid = false;
      switch (geometry)
        {
        case QUADRILATERAL:
          if (this->VolumeElementsOnly)
            {
            continue;
            }
          if (nodesPerElement==4)
            {
            type = VTK_QUAD;
            numberOfCellPoints = this->GhostNodes ? nodesPerElement : 4;
            cellPoints = new int[numberOfCellPoints];
            for (i=0; i<nodesPerElement; i++)
              {
              fscanf(FDNEUTFile, "%d", &cellPoints[i]);
              }
            }
          else if ((nodesPerElement==8) || (nodesPerElement==9))
            {
            type = VTK_QUADRATIC_QUAD;
            numberOfCellPoints = this->GhostNodes ? nodesPerElement : 8;
            cellPoints = new int[numberOfCellPoints];
            fscanf(FDNEUTFile, "%d", &cellPoints[0]);
            fscanf(FDNEUTFile, "%d", &cellPoints[4]);
            fscanf(FDNEUTFile, "%d", &cellPoints[1]);
            fscanf(FDNEUTFile, "%d", &cellPoints[5]);
            fscanf(FDNEUTFile, "%d", &cellPoints[2]);
            fscanf(FDNEUTFile, "%d", &cellPoints[6]);
            fscanf(FDNEUTFile, "%d", &cellPoints[3]);
            fscanf(FDNEUTFile, "%d", &cellPoints[7]);
            if (nodesPerElement==9)
              {
              if (this->GhostNodes)
                {
                fscanf(FDNEUTFile, "%d",&cellPoints[8]);
                }
              else
                {
                fscanf(FDNEUTFile, "%d",&pointBuffer);
                }
              }
            }
          break;
        case TRIANGLE:
          if (this->VolumeElementsOnly)
            {
            continue;
            }
          if (nodesPerElement==3)
            {
            type = VTK_TRIANGLE;
            numberOfCellPoints = this->GhostNodes ? nodesPerElement : 3;
            cellPoints = new int[numberOfCellPoints];
            for (i=0; i<nodesPerElement; i++)
              {
              fscanf(FDNEUTFile, "%d", &cellPoints[i]);
              }
            }
          else if ((nodesPerElement==6) || (nodesPerElement==7))
            {
            type = VTK_QUADRATIC_TRIANGLE;
            numberOfCellPoints = this->GhostNodes ? nodesPerElement : 6;
            cellPoints = new int[numberOfCellPoints];
            fscanf(FDNEUTFile, "%d", &cellPoints[0]);
            fscanf(FDNEUTFile, "%d", &cellPoints[3]);
            fscanf(FDNEUTFile, "%d", &cellPoints[1]);
            fscanf(FDNEUTFile, "%d", &cellPoints[4]);
            fscanf(FDNEUTFile, "%d", &cellPoints[2]);
            fscanf(FDNEUTFile, "%d", &cellPoints[5]);
 
            if (nodesPerElement==7)
              {
              if (this->GhostNodes)
                {
                fscanf(FDNEUTFile, "%d",&cellPoints[6]);
                }
              else
                {
                fscanf(FDNEUTFile, "%d",&pointBuffer);
                }
              }
            }
          break;
        case BRICK:
          if (nodesPerElement==8)
            {
            type = VTK_HEXAHEDRON;
            numberOfCellPoints = this->GhostNodes ? nodesPerElement : 8;
            cellPoints = new int[numberOfCellPoints];
            fscanf(FDNEUTFile, "%d", &cellPoints[0]);
            fscanf(FDNEUTFile, "%d", &cellPoints[1]);
            fscanf(FDNEUTFile, "%d", &cellPoints[3]);
            fscanf(FDNEUTFile, "%d", &cellPoints[2]);
            fscanf(FDNEUTFile, "%d", &cellPoints[4]);
            fscanf(FDNEUTFile, "%d", &cellPoints[5]);
            fscanf(FDNEUTFile, "%d", &cellPoints[7]);
            fscanf(FDNEUTFile, "%d", &cellPoints[6]);
            }
          else if (nodesPerElement==27)
            {
            type = VTK_QUADRATIC_HEXAHEDRON;
            numberOfCellPoints = this->GhostNodes ? nodesPerElement : 20;
            cellPoints = new int[numberOfCellPoints];
            fscanf(FDNEUTFile, "%d", &cellPoints[0]);
            fscanf(FDNEUTFile, "%d", &cellPoints[8]);
            fscanf(FDNEUTFile, "%d", &cellPoints[1]);

            fscanf(FDNEUTFile, "%d", &cellPoints[11]);
            if (this->GhostNodes)
              {
              fscanf(FDNEUTFile, "%d", &cellPoints[24]);
              }
            else
              {
              fscanf(FDNEUTFile, "%d", &pointBuffer);
              }
            fscanf(FDNEUTFile, "%d", &cellPoints[9]);

            fscanf(FDNEUTFile, "%d", &cellPoints[3]);
            fscanf(FDNEUTFile, "%d", &cellPoints[10]);
            fscanf(FDNEUTFile, "%d", &cellPoints[2]);

            fscanf(FDNEUTFile, "%d", &cellPoints[16]);
            if (this->GhostNodes)
              {
              fscanf(FDNEUTFile, "%d", &cellPoints[20]);
              }
            else
              {
              fscanf(FDNEUTFile, "%d", &pointBuffer);
              }
            fscanf(FDNEUTFile, "%d", &cellPoints[17]);

            if (this->GhostNodes)
              {
              fscanf(FDNEUTFile, "%d", &cellPoints[23]);
              fscanf(FDNEUTFile, "%d", &cellPoints[26]);
              fscanf(FDNEUTFile, "%d", &cellPoints[21]);
              }
            else
              {
              fscanf(FDNEUTFile, "%d", &pointBuffer);
              fscanf(FDNEUTFile, "%d", &pointBuffer);
              fscanf(FDNEUTFile, "%d", &pointBuffer);
              }

            fscanf(FDNEUTFile, "%d",&cellPoints[19]);
            if (this->GhostNodes)
              {
              fscanf(FDNEUTFile, "%d", &cellPoints[22]);
              }
            else
              {
              fscanf(FDNEUTFile, "%d", &pointBuffer);
              }
            fscanf(FDNEUTFile, "%d",&cellPoints[18]);

            fscanf(FDNEUTFile, "%d",&cellPoints[4]);
            fscanf(FDNEUTFile, "%d",&cellPoints[12]);
            fscanf(FDNEUTFile, "%d",&cellPoints[5]);

            fscanf(FDNEUTFile, "%d",&cellPoints[15]);
            if (this->GhostNodes)
              {
              fscanf(FDNEUTFile, "%d", &cellPoints[25]);
              }
            else
              {
              fscanf(FDNEUTFile, "%d", &pointBuffer);
              }
            fscanf(FDNEUTFile, "%d",&cellPoints[13]);

            fscanf(FDNEUTFile, "%d",&cellPoints[7]);
            fscanf(FDNEUTFile, "%d",&cellPoints[14]);
            fscanf(FDNEUTFile, "%d",&cellPoints[6]);
            }
          break;
        case TETRAHEDRON:
          if (nodesPerElement==4)
            {
            type = VTK_TETRA;
            numberOfCellPoints = this->GhostNodes ? nodesPerElement : 4;
            cellPoints = new int[numberOfCellPoints];
            for (i=0; i<nodesPerElement; i++)
              {
              fscanf(FDNEUTFile, "%d", &cellPoints[i]);
              }
            }
          else if (nodesPerElement==10)
            {
            type = VTK_QUADRATIC_TETRA;
            numberOfCellPoints = this->GhostNodes ? nodesPerElement : 10;
            cellPoints = new int[numberOfCellPoints];
            fscanf(FDNEUTFile, "%d", &cellPoints[0]);
            fscanf(FDNEUTFile, "%d", &cellPoints[4]);
            fscanf(FDNEUTFile, "%d", &cellPoints[1]);
            fscanf(FDNEUTFile, "%d", &cellPoints[6]);
            fscanf(FDNEUTFile, "%d", &cellPoints[5]);
            fscanf(FDNEUTFile, "%d", &cellPoints[2]);
            fscanf(FDNEUTFile, "%d", &cellPoints[7]);
            fscanf(FDNEUTFile, "%d", &cellPoints[8]);
            fscanf(FDNEUTFile, "%d", &cellPoints[9]);
            fscanf(FDNEUTFile, "%d", &cellPoints[3]);
            }
          break;
        case WEDGE:
          if (nodesPerElement==6)
            {
            type = VTK_WEDGE;
            numberOfCellPoints = this->GhostNodes ? nodesPerElement : 6;
            cellPoints = new int[numberOfCellPoints];
            for (i=0; i<nodesPerElement; i++)
              {
              fscanf(FDNEUTFile, "%d", &cellPoints[i]);
              }
            }
          else if (nodesPerElement==18)
            {
            type = VTK_QUADRATIC_WEDGE;
            numberOfCellPoints = this->GhostNodes ? nodesPerElement : 15;
            cellPoints = new int[numberOfCellPoints];
            fscanf(FDNEUTFile, "%d", &cellPoints[0]);
            fscanf(FDNEUTFile, "%d", &cellPoints[6]);
            fscanf(FDNEUTFile, "%d", &cellPoints[1]);

            fscanf(FDNEUTFile, "%d", &cellPoints[8]);
            fscanf(FDNEUTFile, "%d", &cellPoints[7]);
            fscanf(FDNEUTFile, "%d", &cellPoints[2]);

            fscanf(FDNEUTFile, "%d", &cellPoints[12]);
            if (this->GhostNodes)
              {
              fscanf(FDNEUTFile, "%d", &cellPoints[15]);
              }
            else
              {
              fscanf(FDNEUTFile, "%d", &pointBuffer);
              }
            fscanf(FDNEUTFile, "%d", &cellPoints[13]);

            if (this->GhostNodes)
              {
              fscanf(FDNEUTFile, "%d", &cellPoints[16]);
              fscanf(FDNEUTFile, "%d", &cellPoints[17]);
              }
            else
              {
              fscanf(FDNEUTFile, "%d", &pointBuffer);
              fscanf(FDNEUTFile, "%d", &pointBuffer);
              }
            fscanf(FDNEUTFile, "%d", &cellPoints[14]);

            fscanf(FDNEUTFile, "%d", &cellPoints[3]);
            fscanf(FDNEUTFile, "%d", &cellPoints[9]);
            fscanf(FDNEUTFile, "%d", &cellPoints[4]);

            fscanf(FDNEUTFile, "%d",&cellPoints[11]);
            fscanf(FDNEUTFile, "%d",&cellPoints[10]);
            fscanf(FDNEUTFile, "%d",&cellPoints[5]);
            }
          else if (nodesPerElement==15)
            {
            type = VTK_QUADRATIC_WEDGE;
            numberOfCellPoints = this->GhostNodes ? nodesPerElement : 15;
            cellPoints = new int[numberOfCellPoints];
            fscanf(FDNEUTFile, "%d", &cellPoints[0]);
            fscanf(FDNEUTFile, "%d", &cellPoints[6]);
            fscanf(FDNEUTFile, "%d", &cellPoints[1]);

            fscanf(FDNEUTFile, "%d", &cellPoints[8]);
            fscanf(FDNEUTFile, "%d", &cellPoints[7]);
            fscanf(FDNEUTFile, "%d", &cellPoints[2]);

            fscanf(FDNEUTFile, "%d", &cellPoints[12]);
            fscanf(FDNEUTFile, "%d", &cellPoints[13]);
            fscanf(FDNEUTFile, "%d", &cellPoints[14]);

            fscanf(FDNEUTFile, "%d", &cellPoints[3]);
            fscanf(FDNEUTFile, "%d", &cellPoints[9]);
            fscanf(FDNEUTFile, "%d", &cellPoints[4]);

            fscanf(FDNEUTFile, "%d",&cellPoints[11]);
            fscanf(FDNEUTFile, "%d",&cellPoints[10]);
            fscanf(FDNEUTFile, "%d",&cellPoints[5]);
            }
          break;
        default:
          invalid = true;
        }
#if 0
      entityArray->InsertValue(gridCellArray->GetNumberOfCells()-1,1);
#endif
      if (!invalid)
        {
        typesArray->InsertNextValue(type);
        this->OneToZeroOffset(numberOfCellPoints,cellPoints);
        gridCellArray->InsertNextCell(numberOfCellPoints,cellPoints);
        singleEntityArray->InsertNextValue(entityCounter);
        delete[] cellPoints;
        }
      else
        {
        cout<<"foo"<<endl;
        }
      }
#if 0
    entityArray->Delete();
#endif
    ++entityCounter;
    }

  singleEntityArray->Delete();

  fclose(FDNEUTFile);

  output->SetPoints(points);
  output->SetCells(typesArray->GetPointer(0),gridCellArray);

  points->Delete();
  typesArray->Delete();
  gridCellArray->Delete();

  return 1;
}

void vtkvmtkFDNEUTReader::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

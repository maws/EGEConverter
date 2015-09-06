#include <fbxsdk.h>
#include <vector>
#include <iostream>
#include <fstream> 
#include <stdio.h>
#include <string>

struct Vertex
{
	Vertex() {}
	Vertex(float x, float y, float z) :x(x), y(y), z(z) {}
	float x, y, z;
};

int main(int argc, char *argv[])
{
	// The master vertices list used for exporting
	std::vector<Vertex> meshVertices;

	std::string fileName = argv[1];
	printf("Importing %s \n", fileName.c_str());
	
	// Strip extension
	int extIndex = fileName.find_last_of(".");
	std::string fileNameRaw = fileName.substr(0, extIndex);

	FbxManager* manager = FbxManager::Create();
	FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
	FbxScene* scene = FbxScene::Create(manager, "Scene");

	// Get sdk version info
	int sdkMajor, sdkMinor, sdkRev;
	FbxManager::GetFileFormatVersion(sdkMajor, sdkMinor, sdkRev);

	FbxImporter* importer = FbxImporter::Create(manager, "");
	bool status = importer->Initialize(fileName.c_str(), -1, manager->GetIOSettings());

	// Get file version info
	int fileMajor, fileMinor, fileRev;
	importer->GetFileVersion(fileMajor, fileMinor, fileRev);

	if (!status)
	{
		// Print error
		FbxString error = importer->GetStatus().GetErrorString();
		printf("Failed to initialize fbx importer. \n");
		printf("Error: %s \n", error.Buffer());
	}

	printf("FBX SDK version: %d.%d.%d \n", sdkMajor, sdkMinor, sdkRev);
	if (importer->IsFBX())
	{
		printf("FBX file format version: %d.%d.%d \n", fileMajor, fileMinor, fileRev);

	}

	// Import scene
	status = importer->Import(scene);

	auto rootNode = scene->GetRootNode();
	
	if (rootNode)
	{
		for (int i = 0; i < rootNode->GetChildCount(); ++i)
		{
			auto node = rootNode->GetChild(i);

			FbxNodeAttribute::EType type;
			if (node->GetNodeAttribute() == nullptr)
			{
				printf("Node %d has no attribute \n", i);
			}
			else
			{
				type = node->GetNodeAttribute()->GetAttributeType();

				switch (type)
				{
				default:
					break;
				case FbxNodeAttribute::eMesh:
					{
						FbxMesh* mesh = (FbxMesh*)node->GetNodeAttribute();
						printf("Mesh name: %s \n", (char*)node->GetName());

						auto polyCount = mesh->GetPolygonCount();
						FbxVector4* controlPoints = mesh->GetControlPoints();

						// Loop polys
						for (int poly = 0; poly < polyCount; ++poly)
						{
							auto polySize = mesh->GetPolygonSize(poly);
							for (int polyVertex = 0; polyVertex < polySize; ++polyVertex)
							{
								int controlPointIndex = mesh->GetPolygonVertex(poly, polyVertex);
								FbxVector4 coordinate = controlPoints[controlPointIndex];
								printf("Polygon %d vertex %d has coordinate %f %f %f \n", poly, polyVertex, coordinate[0], coordinate[1], coordinate[2]);
								meshVertices.push_back(Vertex(coordinate[0], coordinate[1], coordinate[2]));
							}
						}
					}
					break;
				}
			}
		}
	}

	printf("Total vertices: %d \n", meshVertices.size());
	printf("Exporting to e3m... \n");
	fileNameRaw.append(".e3m");

	{ // Write
		Vertex verts[24];
		unsigned numVerts[1] = { 24 };
		FILE* file;
		file = fopen(fileNameRaw.c_str(), "wb");
		for (int i = 0; i < meshVertices.size(); ++i)
		{
			verts[i] = meshVertices[i];
		}

		// Write the number of vertices
		fwrite(numVerts, sizeof(unsigned), 1, file);
		
		// Write vertices
		fwrite(verts, sizeof(Vertex), 24, file);

		fclose(file);
	}
	{ // Read
		Vertex buffer[24];
		unsigned numVerts[1];
		FILE* file;
		file = fopen(fileNameRaw.c_str(), "rb");
		fread(numVerts, sizeof(unsigned), 1, file);
		fread(buffer, sizeof(Vertex), numVerts[0], file);
		fclose(file);
	}

	printf("...Finished: Cleaning up. \n");
	manager->Destroy();
	return 0;
}

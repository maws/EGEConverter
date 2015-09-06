#include <fbxsdk.h>
#include <iostream>
#include <fstream> 
#include <stdio.h>
#include <string>


using namespace fbxsdk_2015_1;

int main(int argc, char *argv[])
{
	// The master vertices list used for exporting
	float* vertexBuffer = nullptr; // Allocate when we know length
	unsigned numVerts = 0;

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
		
						numVerts = mesh->GetControlPointsCount();
						vertexBuffer = new float[numVerts * 3];
						FbxVector4* vertexArray = mesh->GetControlPoints();

						for (unsigned j = 0; j < numVerts; ++j)
						{
							printf("Vertex %d has coordinates %f %f %f \n", j, vertexArray[j][0], vertexArray[j][1], vertexArray[j][2]);
							float verts[3] = { vertexArray[j][0], vertexArray[j][1], vertexArray[j][2] };
							int dstIndex = j * 3;
							memcpy(&vertexBuffer[dstIndex], &verts, sizeof(verts));
						}
					}
					break;
				}
			}
		}
	}

	printf("Total vertices: %d \n", numVerts);
	printf("Exporting to e3m... \n");
	fileNameRaw.append(".e3m");

	{ // Write
		FILE* file;
		file = fopen(fileNameRaw.c_str(), "wb");
		// Write the number of vertices
		fwrite(&numVerts, sizeof(unsigned), 1, file);
		// Write vertices
		fwrite(vertexBuffer, sizeof(float), numVerts * 3, file);
		fclose(file);
	}
	{ // Read
		float* buffer = new float[numVerts];
		FILE* file;
		file = fopen(fileNameRaw.c_str(), "rb");
		fread(&numVerts, sizeof(unsigned), 1, file);
		fread(buffer, sizeof(float), numVerts * 3, file);
		fclose(file);
		delete buffer;
	}

	printf("...Finished: Cleaning up. \n");
	delete vertexBuffer;
	manager->Destroy();
	return 0;
}

//------------------------------------------------------------------------------
//  gridrtplugin.cc
//  (C) 2012-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "terrainplugin.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/memoryvertexbufferloader.h"
#include "coregraphics/memoryindexbufferloader.h"
#include "coregraphics/transformdevice.h"
#include "coregraphics/displaydevice.h"
#include "resources/resourcemanager.h"

using namespace Math;
using namespace CoreGraphics;
using namespace Resources;
namespace Terrain
{
	__ImplementClass(Terrain::TerrainRTPlugin, 'TRRT', RenderModules::RTPlugin);

	//------------------------------------------------------------------------------
	/**
	*/
	TerrainRTPlugin::TerrainRTPlugin() :
		visible(true),
		gridSize(1)
	{
		// empty
	}

	//------------------------------------------------------------------------------
	/**
	*/
	TerrainRTPlugin::~TerrainRTPlugin()
	{
		// empty
	}

	//------------------------------------------------------------------------------
	/**
	*/
	void
		TerrainRTPlugin::OnRegister()
	{
		TerrainInit();
	}

	//------------------------------------------------------------------------------
	/**
	*/
	void
		TerrainRTPlugin::OnUnregister()
	{
		this->tex = 0;
		this->ibo->Unload();
		this->ibo = 0;
		this->vbo->Unload();
		this->vbo = 0;

		this->gridSizeVar = 0;
		this->gridTexVar = 0;
		this->shader = 0;
	}

	//------------------------------------------------------------------------------
	/**
	*/
	void
		TerrainRTPlugin::OnRenderFrameBatch(const Ptr<Frame::FrameBatch>& frameBatch)
	{
		if (FrameBatchType::Shapes == frameBatch->GetType() && this->visible)
		{
			DrawTerrain();
		}
	}

	void TerrainRTPlugin::TerrainInit()
	{
		device = RenderDevice::Instance();
		trans = TransformDevice::Instance();

		LoadShader();

		GenerateTerrainBasedOnResolution(1024, 1024);

		SetUpVBO(this->vertices, this->indices, 1024, 1024);
	}

	void TerrainRTPlugin::LoadShader()
	{
		// create new shader
		this->shader = ShaderServer::Instance()->GetShader("shd:my_simple");
		//this->gridSizeVar = this->shader->GetVariableByName("GridSize");
		this->gridTexVar = this->shader->GetVariableByName("HeightMap");

		// load texture
		this->tex = ResourceManager::Instance()->CreateManagedResource(Texture::RTTI, "tex:system/grid.dds").downcast<ManagedTexture>();
	}

	void TerrainRTPlugin::GenerateTerrainBasedOnResolution(int width, int height)
	{
		int vertexCount = width * height;
		int squares = vertexCount / 4;
		int triangles = squares * 2;
		int indicesCount = triangles * 3;
		// Create the structure to hold the mesh data.
		vertices = new Math::vector[vertexCount]; //3 floats per vertex
		indices = new int[indicesCount];

		// Generate indices for specified resolution
		//we store columns in the array
		//current column is 0
		//current row is 0

		//since i store the points column wise the next column starts at index = current column * height

		//so how do we traverse?
		//we traverse with squares

		//we can traverse column wise
		//so first loop loops columns
		//second loop loops rows
		//first is column 0
		//first row is 0
		//then row is 1
		//we never do the last row nor last column, we don't do that with borders since they are already a part of pre-border edges

		//face 1
		//vertex 0 is current column and current row
		//vertex 1 is current column and current row + 1
		//vertex 2 is current column + 1 and current row + 1

		//face 2
		//vertex 0 is current column + 1 and current row + 1 i.e. same as face 1 vertex 2
		//vertex 1 is current column + 1 and current row
		//vertex 2 is current column and current row

		//indices are 0 1 4, 5 4 0
		//let's walk around the square
		//must traverse triangle vertices in same direction for all triangles f.ex. all face vertices are traversed counter-clockwise
		int index = 0;
		for (int col = 0; col < width; col++)
		{
			for (int row = 0; row < height; row++)
			{
				//since i store the points column wise the next column starts at index = current column * height
				int currentColumn = height * col;
				index = (currentColumn)+row; //
				vertices[index].set((float)row, 0.f, (float)col);

				//we never do the last row nor last column, we don't do that with borders since they are already a part border faces that were build in previous loop
				if (col == width - 1 || row == height - 1) continue; //this might be more expensive than writing another for loop set just for indices

				//face 1
				//vertex 0 is current column and current row
				//vertex 1 is current column and current row + 1
				//vertex 2 is current column + 1 and current row + 1
				int nextColumn = height * (col + 1); //or currentColumn + height //will use that later
				indices[index + 0] = currentColumn + row;
				indices[index + 1] = currentColumn + row + 1;
				indices[index + 2] = nextColumn + row + 1; //we need to calculate the next column here
				//face 2
				//vertex 0 is current column + 1 and current row + 1 i.e. same as face 1 vertex 2
				//vertex 1 is current column + 1 and current row
				//vertex 2 is current column and current row i.e. same as face 1 vertex 1
				indices[index + 3] = indices[index + 2];
				indices[index + 4] = nextColumn + row;
				indices[index + 5] = indices[index];
			}
		}
	}

	void TerrainRTPlugin::SetUpVBO(Math::vector* terrainMesh, int* indices, int width, int height)
	{
		// setup VBO
		Util::Array<VertexComponent> components;
		components.Append(VertexComponent(VertexComponent::Position, 0, VertexComponent::Float4, 0));
		Ptr<MemoryVertexBufferLoader> vboLoader = MemoryVertexBufferLoader::Create();
		int vertCount = width*height;
		vboLoader->Setup(components, vertCount, terrainMesh, vertCount*sizeof(Math::vector), VertexBuffer::UsageImmutable, VertexBuffer::AccessNone);

		this->vbo = VertexBuffer::Create();
		this->vbo->SetLoader(vboLoader.upcast<ResourceLoader>());
		this->vbo->SetAsyncEnabled(false);
		this->vbo->Load();
		n_assert(this->vbo->IsLoaded());
		this->vbo->SetLoader(NULL);

		Ptr<MemoryIndexBufferLoader> iboLoader = MemoryIndexBufferLoader::Create();
		//4 vertices per square
		int squares = vertCount / 4;
		int triangles = squares * 2;
		int indicesCount = triangles * 3;
		iboLoader->Setup(IndexType::Index32, indicesCount, indices, indicesCount*sizeof(int));

		this->ibo = IndexBuffer::Create();
		this->ibo->SetLoader(iboLoader.upcast<ResourceLoader>());
		this->ibo->SetAsyncEnabled(false);
		this->ibo->Load();
		n_assert(this->ibo->IsLoaded());
		this->ibo->SetLoader(NULL);

		// setup ibo
		this->vertexLayout = this->vbo->GetVertexLayout();
		this->vertexLayout->SetIndexBuffer(this->ibo);

		this->primitive.SetBaseIndex(0);
		this->primitive.SetNumVertices(vertCount);
		this->primitive.SetBaseIndex(0);
		this->primitive.SetNumIndices(indicesCount);
		this->primitive.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
	}

	void TerrainRTPlugin::EnableShader()
	{
		// start pass
		this->shader->Apply();

		// set variables
		this->shader->BeginUpdate();
		//this->gridSizeVar->SetFloat(this->gridSize);
		this->gridTexVar->SetTexture(this->tex->GetTexture());
		this->shader->EndUpdate();
		this->shader->Commit();
	}

	void TerrainRTPlugin::EnableTerrain()
	{
		device->SetStreamSource(0, this->vbo, 0);
		device->SetVertexLayout(this->vertexLayout);
		device->SetIndexBuffer(this->ibo);
		device->SetPrimitiveGroup(this->primitive);
	}

	void TerrainRTPlugin::DrawTerrain()
	{
		EnableShader();
		EnableTerrain();
		device->Draw();
	}

} // namespace Grid
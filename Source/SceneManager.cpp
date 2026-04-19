///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// intialize textures
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;

}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/
void SceneManager::LoadSceneTextures()
{
	// Load wood texture for the desk surface plane
	CreateGLTexture("textures/wood.jpg", "wood");

	// Load ceramic texture for the mug body and handle
	CreateGLTexture("textures/ceramic.jpg", "ceramic");

	// Bind all loaded textures to their texture slots
	BindGLTextures();
}
void SceneManager::DefineObjectMaterials()
{
	// Wood material for the desk plane
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.5f, 0.35f, 0.2f);
	woodMaterial.specularColor = glm::vec3(0.15f, 0.15f, 0.15f);
	woodMaterial.shininess = 12.0f;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	// Ceramic material for the mug body and handle
	OBJECT_MATERIAL ceramicMaterial;
	ceramicMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	ceramicMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	ceramicMaterial.shininess = 32.0f;
	ceramicMaterial.tag = "ceramic";
	m_objectMaterials.push_back(ceramicMaterial);
}
void SceneManager::SetupSceneLights()
{
	// Turn on custom lighting
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Keep other light types off for this milestone
	m_pShaderManager->setBoolValue("directionalLight.bActive", false);
	m_pShaderManager->setBoolValue("spotLight.bActive", false);

	// Main white point light above the desk
	m_pShaderManager->setVec3Value("pointLights[0].position", -3.0f, 6.0f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.08f, 0.08f, 0.08f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.85f, 0.85f, 0.85f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.35f, 0.35f, 0.35f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Secondary blue fill light so the scene is not flat or fully shadowed
	m_pShaderManager->setVec3Value("pointLights[1].position", 3.0f, 4.0f, -2.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.03f, 0.03f, 0.08f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.30f, 0.45f, 0.75f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.20f, 0.25f, 0.40f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// Turn off unused point lights
	m_pShaderManager->setBoolValue("pointLights[2].bActive", false);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", false);
}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Load textures used in the scene
	LoadSceneTextures();

	// Define material properties for textured objects
	DefineObjectMaterials();

	// Configure light sources for the scene
	SetupSceneLights();

	// Load meshes used in the scene
	m_basicMeshes->LoadPlaneMesh(); 
	m_basicMeshes->LoadCylinderMesh(); 
	m_basicMeshes->LoadTorusMesh(); 
	m_basicMeshes->LoadBoxMesh(); 
	m_basicMeshes->LoadConeMesh();
}
void SceneManager::RenderPencil(
	glm::vec3 bodyPosition,
	float bodyHeight,
	glm::vec3 bodyColor,
	float rotX,
	float rotY,
	float rotZ)
{
	// ---- pencil body ----
	SetTransformations(
		glm::vec3(0.04f, bodyHeight, 0.04f),
		rotX, rotY, rotZ,
		bodyPosition);

	SetShaderColor(bodyColor.r, bodyColor.g, bodyColor.b, 1.0f);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawCylinderMesh();

	// ---- auto-calculate tip position ----
	// The cylinder's top cap sits at +bodyHeight along its local Y axis.
	// We rotate that offset by the same Euler angles, then add to bodyPosition.
	float rx = glm::radians(rotX);
	float ry = glm::radians(rotY);
	float rz = glm::radians(rotZ);

	glm::mat4 rot = glm::rotate(rz, glm::vec3(0, 0, 1))
		* glm::rotate(ry, glm::vec3(0, 1, 0))
		* glm::rotate(rx, glm::vec3(1, 0, 0));

	// local-space offset to the top of the cylinder
	glm::vec4 localTop = glm::vec4(0.0f, bodyHeight, 0.0f, 0.0f);
	glm::vec3 tipOffset = glm::vec3(rot * localTop);
	glm::vec3 tipPosition = bodyPosition + tipOffset;

	// ---- pencil tip (cone) ----
	SetTransformations(
		glm::vec3(0.04f, 0.12f, 0.05f),
		rotX, rotY, rotZ,
		tipPosition);

	SetShaderColor(0.78f, 0.62f, 0.40f, 1.0f);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawConeMesh();
}
/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh

	// Desk surface - Plane
	// Desk surface for all future objects to sit on.
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);

	// Apply wood texture to desk surface with tiling
	SetShaderTexture("wood");
	SetTextureUVScale(4.0f, 4.0f);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	/****************************************************************/
	// Mug-Body
	// Body of the dark gray mug from the reference image.
	// scaled to appear wide

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh

	
	scaleXYZ = glm::vec3(0.4f, 0.5f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(.2f, .2f, .2f, 1.0f);

	// Apply ceramic texture to mug handle to match body
	SetShaderTexture("ceramic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("ceramic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/****************************************************************/
	// Mug-handle
	// represents the handle of the mug using torus shape.
	// flattened on Z to make thinner handle ring

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 0.2f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.6f, 0.25f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(.2f, .2f, .2f, 1.0f);

	SetShaderTexture("ceramic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("ceramic");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/


		/****************************************************************/
	// Monitor frame
	// Main outer body of the monitor

	scaleXYZ = glm::vec3(3.8f, 2.2f, 0.15f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.8f, 1.55f, -1.2f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.05f, 0.05f, 0.05f, 1.0f);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/****************************************************************/
	// Monitor screen
	// Bright inner panel of the monitor

	scaleXYZ = glm::vec3(3.35f, 1.8f, 0.08f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.8f, 1.65f, -1.08f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.95f, 0.95f, 0.95f, 1.0f); // powered on
	SetShaderColor(0.7f, 0.75f, 0.8f, 1.0f); // off
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/****************************************************************/
	// Monitor stand stem
	// Vertical support between monitor and base

	scaleXYZ = glm::vec3(0.28f, 1.0f, 0.18f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.8f, 0.55f, -1.2f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.78f, 0.78f, 0.78f, 1.0f);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/****************************************************************/
	// Monitor base
	// Bottom support resting on the desk

	scaleXYZ = glm::vec3(1.2f, 0.08f, 0.6f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.8f, 0.06f, -1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.82f, 0.82f, 0.82f, 1.0f);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	
	/****************************************************************/
	// Keyboard base
	// Simple low-profile keyboard placed in front of the monitor

	scaleXYZ = glm::vec3(2.4f, 0.08f, 0.75f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.7f, 0.06f, 1.2f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.94f, 0.94f, 0.94f, 1.0f);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/****************************************************************/
	// Mouse body
	// Simple computer mouse placed to the right of the keyboard

	scaleXYZ = glm::vec3(0.24f, 0.08f, 0.36f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.6f, 0.10f, 1.15f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.96f, 0.96f, 0.96f, 1.0f);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	
	/****************************************************************/
	// Pencil cup
	// Container holding pencils on right side of desk

	scaleXYZ = glm::vec3(0.35f, 0.55f, 0.35f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(3.2f, 0.0f, -0.2f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.85f, 0.85f, 0.85f, 1.0f);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/

	/****************************************************************/
	// Pencil 1 - leans left, tilted forward
	RenderPencil(
		glm::vec3(3.18f, 0.38f, -0.12f),
		0.75f,
		glm::vec3(0.20f, 0.60f, 0.90f),
		-10.0f, -25.0f, 8.0f);

	// Pencil 2 - leans right, tilted back  
	RenderPencil(
		glm::vec3(3.32f, 0.38f, -0.05f),
		0.75f,
		glm::vec3(0.90f, 0.30f, 0.20f),  // red pencil
		-14.0f, 10.0f, -6.0f);

	// Pencil 3 - fairly upright, slightly off-center
	RenderPencil(
		glm::vec3(3.24f, 0.38f, -0.22f),
		0.80f,
		glm::vec3(0.20f, 0.75f, 0.30f),  // green pencil
		-8.0f, 5.0f, 12.0f);
	/****************************************************************/
}

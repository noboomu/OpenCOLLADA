/*
    Copyright (c) 2008 NetAllied Systems GmbH

    This file is part of COLLADADomHelper.

    Licensed under the MIT Open Source License, 
    for details please see LICENSE file or the website
    http://www.opensource.org/licenses/mit-license.php
*/

#include "COLLADADHStableHeaders.h"
#include "COLLADADHVisualSceneLoader.h"
#include "COLLADAFWVisualScene.h"
#include "COLLADAFWRotate.h"
#include "COLLADAFWTranslate.h"
#include "COLLADAFWScale.h"
#include "COLLADAFWMatrix.h"
#include "COLLADAFWMathMatrix4.h"

namespace COLLADADH
{
	//--------------------------------------------------------------------
	VisualSceneLoader::VisualSceneLoader(Loader* colladaLoader, domVisual_scene* colladaVisualScene)
		 : mColladaVisualScene(colladaVisualScene),
		 mColladaLoader(colladaLoader)
	{
	}
	
	//--------------------------------------------------------------------
	VisualSceneLoader::~VisualSceneLoader()
	{
	}

	COLLADAFW::VisualScene* VisualSceneLoader::load()
	{
 		mVisualScene = new COLLADAFW::VisualScene();
 
 		const domNode_Array& nodeArray = mColladaVisualScene->getNode_array();
 
 		mVisualScene->getRootNodes().allocateMemory( nodeArray.getCount() );
 
 		COLLADADH::NodeTraverser traverser(*mColladaVisualScene);
 		if ( !traverser.traverse(*this) )
 		{
 			delete mVisualScene;
 			return 0;
 		}
 
 		return mVisualScene;
	}

	bool VisualSceneLoader::detectedInstanceGeometry( domInstance_geometry& instance, int level )
	{

		return true;
	}

	bool VisualSceneLoader::preDetectedNode( domNode& node, int level )
	{
		COLLADAFW::Node* newNode = new COLLADAFW::Node();

		// Count the transform elements
		daeElementRefArray& children = node.getChildren();
		size_t childrenCount = children.getCount();
		size_t transformationCount = 0;
		for ( size_t i = 0; i < childrenCount; ++i )
		{
			daeElementRef element = children.get( i );

			if (   daeSafeCast<domRotate>( element.cast() ) 
				|| daeSafeCast<domTranslate>( element.cast() )
				|| daeSafeCast<domScale>( element.cast() )
				|| daeSafeCast<domMatrix>( element.cast() )
				)
			{
				transformationCount++;
			}
		}

		// Fill in the transform elemenst
		newNode->getTransformations().allocateMemory(transformationCount);

		for ( size_t i = 0; i < childrenCount; ++i )
		{
			daeElementRef element = children.get( i );
			domRotate* colladaRotate = daeSafeCast<domRotate>( element.cast() );
			domTranslate* colladaTranslate = daeSafeCast<domTranslate>( element.cast() );
			domScale* colladaScale = daeSafeCast<domScale>( element.cast() );
			domMatrix* colladaMatrix = daeSafeCast<domMatrix>( element.cast() );

			COLLADAFW::TransformationArray& transformations = newNode->getTransformations();

			if ( colladaRotate )
			{
				const domFloat4& values = colladaRotate->getValue();
				transformations.append( new COLLADAFW::Rotate(values.get(0), values.get(1), values.get(2), values.get(3)) );
			}
			else if ( colladaTranslate )
			{
				const domFloat3& values = colladaTranslate->getValue();
				transformations.append( new COLLADAFW::Translate(values.get(0), values.get(1), values.get(2)) );
			}
			else if ( colladaScale )
			{
				const domFloat3& values = colladaScale->getValue();
				transformations.append( new COLLADAFW::Scale(values.get(0), values.get(1), values.get(2)) );
			}
			else if ( colladaMatrix )
			{
				const domFloat4x4& values = colladaMatrix->getValue();
				transformations.append( new COLLADAFW::Matrix(COLLADAFW::Math::Matrix4((double*)(values.getRawData()))) );
			}
		}


		/** Allocate memory for instance geometry.*/
		newNode->getInstanceGeometries().allocateMemory(node.getInstance_geometry_array().getCount());

		/** Allocate memory for children.*/
		newNode->getChildNodes().allocateMemory(node.getNode_array().getCount());

		if ( mNodeStack.empty() )
		{
			// we are a direct child of the visual scene
			mVisualScene->getRootNodes().append(newNode);
		}
		else
		{
			//we are a child of another node
			COLLADAFW::Node* parentNode = mNodeStack.top();
			parentNode->getChildNodes().append(newNode);
		}
		mNodeStack.push(newNode);
		return true;
	}

	bool VisualSceneLoader::postDetectedNode( domNode& node, int level )
	{
		mNodeStack.pop();
		return true;
	}
} // namespace COLLADA
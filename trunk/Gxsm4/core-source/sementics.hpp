/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

#pragma once

namespace semantic
{
        namespace s3d_text
        {
                enum type
                        {
                                POSITION = 0,
                                TEXCOORD = 1
                        };
        };
        namespace ico
        {
                enum type
                        {
                                POSITION = 0,
                                LATTICE = 1
                        };
        };
	namespace buffer
	{
		enum type
		{
			STATIC = 0,
			DYNAMIC = 1
		};
	};

	namespace uniform
	{
		enum type
		{
			MATERIAL = 0,
			TRANSFORM0 = 1,
			TRANSFORM1 = 2,
			INDIRECTION = 3,
			CONSTANT = 0,
			PER_FRAME = 1,
			PER_PASS = 2,
			LIGHT = 3
		};
	};

	namespace sampler
	{
		enum type
		{
			DIFFUSE		= 0,
			POSITION	= 4,
			TEXCOORD	= 5,
			COLOR		= 6
		};
	}//namespace sampler

	namespace image
	{
		enum type
		{
			DIFFUSE = 0,
			PICKING = 1
		};
	}//namesapce image

	namespace attr
	{
		enum type
		{
			POSITION = 0,
			NORMAL	 = 1,
			COLOR	 = 3,
			TEXCOORD = 4,
			DRAW_ID  = 5
		};
	}//namespace attr

        namespace triangles
	{
                enum type
                {
                        INDICES  = 0,
                        POSITION = 1,
                        NORMALS  = 2
                };
                
	}
        namespace indices
	{
                enum type
                {
                        INDICES = 0
                };
                
	}
  
	namespace vert
	{
		enum type
		{
			POSITION = 0,
			COLOR	 = 3,
			TEXCOORD = 4,
			INSTANCE = 7
		};
	}//namespace vert

	namespace frag
	{
		enum type
		{
			COLOR	= 0,
			RED		= 0,
			GREEN	= 1,
			BLUE	= 2,
			ALPHA	= 0
		};
	}//namespace frag

	namespace renderbuffer
	{
		enum type
		{
			DEPTH,
			COLOR0
		};
	}//namespace renderbuffer

	namespace storage
	{
		enum type
		{
			VERTEX	= 0
		};
	}//namespace storage
}//namespace semantic


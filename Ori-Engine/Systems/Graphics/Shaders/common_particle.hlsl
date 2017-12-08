/*	=====================================================================================================
		Common
		Particle Rendering
	=====================================================================================================	*/

/*	=====================================================================================================
		Billboard	http://www.lighthouse3d.com/opengl/billboarding/billboardingtut.pdf
	=====================================================================================================	*/	/*
		Cylindrical
	-----------------------------------------------------------------------------------------------------	*/
void Billboard_ApproximateCylindrical(inout matrix<float, 4, 4> world_view_matrix)
{
	// Right vector is camera right
	world_view_matrix._11 = 1.0f;
	world_view_matrix._21 = 0.0f;
	world_view_matrix._31 = 0.0f;
	// Up vector is unchanged
	// lookAt vector is to camera
	world_view_matrix._13 = 0.0f;
	world_view_matrix._23 = 0.0f;
	world_view_matrix._33 = 1.0f;
}

//void Billboard_Cylindrical() { }

/*	-----------------------------------------------------------------------------------------------------
		Spherical
	-----------------------------------------------------------------------------------------------------	*/
void Billboard_ApproximateSpherical(inout matrix<float, 4, 4> world_view_matrix)
{
	// Right vector is camera right
	world_view_matrix._11 = 1.0f;
	world_view_matrix._21 = 0.0f;
	world_view_matrix._31 = 0.0f;
	// Up vector is camera up
	world_view_matrix._12 = 0.0f;
	world_view_matrix._22 = 1.0f;
	world_view_matrix._32 = 0.0f;
	// lookAt vector is reverse camera direction
	world_view_matrix._13 = 0.0f;
	world_view_matrix._23 = 0.0f;
	world_view_matrix._33 = 1.0f;
}

//void Billboard_Spherical() { }
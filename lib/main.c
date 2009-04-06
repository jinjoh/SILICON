/*                                                                              
                                                                                
This file is part of the IC reverse engineering tool degate.                    
                                                                                
Copyright 2008, 2009 by Martin Schobert                                         
                                                                                
Degate is free software: you can redistribute it and/or modify                  
it under the terms of the GNU General Public License as published by            
the Free Software Foundation, either version 3 of the License, or               
any later version.                                                              
                                                                                
Degate is distributed in the hope that it will be useful,                       
but WITHOUT ANY WARRANTY; without even the implied warranty of                  
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                   
GNU General Public License for more details.                                    
                                                                                
You should have received a copy of the GNU General Public License               
along with degate. If not, see <http://www.gnu.org/licenses/>.                  
                                                                                
*/

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "renderer.h"
#include "project.h"
#include "img_algorithms.h"
#include "globals.h"

#define PROJECT_DIR "/Volumes/GPlatte/degate_temp"
#define RENDERED_IMAGE "/Volumes/GPlatte/degate_temp/foo.raw"

#define SIZE_X 5000
#define SIZE_Y 2200

#define SHOW_SIZE_X SIZE_X
#define SHOW_SIZE_Y SIZE_Y


int main(void) {

  project_t  *project = project_create(PROJECT_DIR, SIZE_X, SIZE_Y, 2);
	if(!project_import(project)) {
		puts("importing data into project failed\n");
		exit(0);
	}

	// running algorithm
	puts("run grid matching");
	
	if(!imgalgo_run_grid_matching(project->matching_images[1], PROJECT_DIR, 128)) {
		puts("matching algorithms failed");
		exit(0);
	}


	puts("run matching");
	if(!imgalgo_run_object_matching(project->matching_images[1], project->lmodel, 1, PROJECT_DIR, SIZE_X, SIZE_Y, 4, TRUE, TRUE, 128)) {
		puts("matching algorithms failed");
		exit(0);
	}
	
	
	
	// rendering
	puts("rendering ...");
	
	char * rendering_buf = malloc(SHOW_SIZE_X * SHOW_SIZE_Y * BYTES_PER_PIXEL);
	render_region(project->renderer, rendering_buf, 1, 0, 0, SIZE_X, SIZE_Y, SHOW_SIZE_X, SHOW_SIZE_Y);

	puts("writing data");
	if(!renderer_write_image(rendering_buf, SHOW_SIZE_X, SHOW_SIZE_Y, RENDERED_IMAGE)) {
		puts("can't write rendered image");
		exit(0);
	}
	
	project_destroy(project);

	return 0;
}

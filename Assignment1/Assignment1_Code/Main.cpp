//#define NDEBUG

#include "jbutil.h"
#include <vector>
#include <limits>
#include <istream>
#include <cmath>
#include <string>

//Paramters for the algorithm: macroblock width and height and search area parameters
int block_width = 8;
int block_height = 8;
int search_vertical = 8;
int search_horizontal = 8;

//Struct used to define a macroblock
typedef struct
{
	jbutil::image<int> block;				//the part of an image which constitutes the block
	int block_location_x;					//top left pixel co ordinates of macroblock (relative to frame)
	int block_location_y;
	int search_location_x_start;			//pixel co ordinates of search area (relative to frame)
	int search_location_x_stop;
	int search_location_y_start;
	int search_location_y_stop;
	int motion_vector_x;					//motion vector
	int motion_vector_y;
}Macroblock;

//Function used to create an image from a range in a given image
//Inputs: image from where to get range, image which is to be set (will be overwritten with the new range),
//        image parameters: channel start and stop, column start and stop,
//        row start and stop, top left pixel co-ordinates of where to set the range
//Output: None
void Set_Image_Range(jbutil::image<int> &input, jbutil::image<int> &output, int channel_start, int channel_stop, int col_start, int col_stop, int row_start, int row_stop)
{
	jbutil::image<int> new_output((row_stop-row_start), (col_stop-col_start),(channel_stop-channel_start)); //define a new image

	int count_channel = 0;
	for(int channel = channel_start; channel<channel_stop; channel++)		//for the defined channels
	{
		int count_row = 0;
		for (int row = row_start; row<row_stop; row++)						//for the range of rows given
		{
			int count_col = 0;
			for(int col = col_start; col<col_stop; col++)					//for the range of columns given
			{
				new_output(count_channel,count_row,count_col) = input(channel,row,col);	//set the new image
				count_col++;
			}
			count_row++;
		}
		count_channel++;
	}
	output = new_output;										//set the old image to the new image defined above
}

//Function used to modify a range in a given image
//Inputs: image from where to get range, image where to set range, image parameters: channel start and stop, column start and stop,
//        row start and stop, top left pixel co-ordinates of where to set the range
//Output: None
void Modify_Image_Range(jbutil::image<int> &input, jbutil::image<int> &output, int channel_start, int channel_stop, int col_start, int col_stop, int row_start, int row_stop, int output_col, int output_row)
{
	for(int channel = channel_start; channel<channel_stop; channel++)	//for the defined channels
	{
		int row_count = 0;												//for the defined rows
		for (int row = row_start; row<row_stop; row++)
		{
			int col_count = 0;
			for(int col = col_start; col<col_stop; col++)				//for the defined columns
			{
				output(channel,output_row+row_count,output_col+col_count) = input(channel,row,col);	//set the range in the output image
				col_count++;
			}
			row_count++;
		}
	}
}

//Function used to calculate the Mean Square Error between 2 blocks
//Inputs:  the 2 blocks to compare
//Outputs: the MSE value
float MSE(jbutil::image<int> &Block_1,jbutil::image<int> &Block_2)
{
	float MSE=0.0;	//start with an MSE of 0
	for (int channel = 0; channel<Block_1.channels(); channel++)	//for every channel
	{
		for (int row = 0; row<Block_1.get_rows(); row++)			//for every row
		{
			for (int col = 0; col<Block_1.get_cols(); col++)		//and for every column
			{
				MSE = MSE + float(pow((Block_1(channel,row,col) - Block_2(channel,row,col)),2));	//calculate the MSE
			}
		}
	}
	MSE = MSE / float(Block_1.channels()*Block_1.get_rows()*Block_1.get_cols());		//normalize the MSE
	return MSE;
}

//Function used to set the co-ordinates of a search area for a macroblock
//Inputs:  Macroblock whose search area will be set, the bounds of an image: max_cols and max_rows,
//         the search area parameters: search_vert and search_horiz, the top left co-ordinates from
//         where the search area should start being defined: centre_col, centre_row
//Outputs: None
void Set_Search_Area(Macroblock& this_macroblock, int max_cols, int max_rows, int search_horiz, int search_vert, int centre_col, int centre_row)
{
	this_macroblock.search_location_x_start = centre_col - search_horiz;	//set the top left column co-ordinate
	if (this_macroblock.search_location_x_start < 0)						//check that it is not out of bounds
	{
		this_macroblock.search_location_x_start = 0;						//if it is, set it to a default value
	}


	this_macroblock.search_location_x_stop = centre_col + block_width + search_horiz;  //set the final pixel's column co-ordinate
	if(this_macroblock.search_location_x_stop > max_cols)					//check that it is not out of bounds
	{
		this_macroblock.search_location_x_stop = max_cols;					//if it is, set it to a default value
	}


	this_macroblock.search_location_y_start = centre_row - search_vert;		//set the top left row co-ordinate
	if(this_macroblock.search_location_y_start < 0)							//check that it is not out of bounds
	{
		this_macroblock.search_location_y_start = 0;						//if it is, set it to a default value
	}

	this_macroblock.search_location_y_stop = centre_row + block_height + search_vert;  //set the final pixel's row co-ordinate
	if(this_macroblock.search_location_y_stop > max_rows)					//check that it is not out of bounds
	{
		this_macroblock.search_location_y_stop = max_rows;					//if it is, set it to a default value
	}

	/*std::cout<<"X_Start = " <<this_macroblock.search_location_x_start<<std::endl<<std::flush;
	std::cout<<"X_Stop  = " <<this_macroblock.search_location_x_stop<<std::endl<<std::flush;
	std::cout<<"Y_Start = " <<this_macroblock.search_location_y_start<<std::endl<<std::flush;
	std::cout<<"Y_Stop  = " <<this_macroblock.search_location_y_stop<<std::endl<<std::flush;*/

}

//Function used to segment an image into macroblocks
//Inpust: Image to be segmented, Array of marcoblocks to store each macroblock
//Outputs: None
void Set_Macroblocks(jbutil::image<int> &current_frame, std::vector<Macroblock>& macroblock_array)
{
	//Get the number of macroblocks
	int macroblocks_horiz = current_frame.get_cols()/block_width;
	int macroblocks_vert  = current_frame.get_rows()/block_height;
		for (int vert = 0; vert<macroblocks_vert; vert++)
		{
			for (int horiz = 0; horiz<macroblocks_horiz; horiz++)
			{
				Macroblock this_macroblock;

				//Set the macroblock location relative to the actual frame
				this_macroblock.block_location_x = horiz*block_width;
				this_macroblock.block_location_y = vert*block_height;

				//Set the macroblock image
				Set_Image_Range(current_frame, this_macroblock.block, 0, current_frame.channels(), horiz*block_width, (horiz+1)*block_width, vert*block_height, (vert+1)*block_height);

				//Set the macroblock search area co-ordinates
				Set_Search_Area(this_macroblock, current_frame.get_cols(), current_frame.get_rows(), search_horizontal, search_vertical, this_macroblock.block_location_x, this_macroblock.block_location_y);

				//Add the macroblock to the marcoblock array
				macroblock_array.push_back(this_macroblock);
			}
		}
}

//Function to reconstruct a frame from a reference frame given the motion vectors in a macroblock array
//Inputs: Reference Frame, Array of Macroblocks containing motion vectors, Reference Frame object to be modified
//Outputs:None
void Reconstruct_Frame(jbutil::image<int> &prev_frame, std::vector<Macroblock>& macroblock_array, jbutil::image<int> &reconstructed_frame)
{
	//iterate through every marcroblock
	for(int macroblock = 0; macroblock<macroblock_array.size(); macroblock++)
	{
		//set the start and stop co-ordinates of the area to be taken from the reference frame
		int x_start = macroblock_array[macroblock].block_location_x+macroblock_array[macroblock].motion_vector_x;
		int x_stop = x_start + block_width;

		int y_start = macroblock_array[macroblock].block_location_y+macroblock_array[macroblock].motion_vector_y;
		int y_stop = y_start + block_height;

		//std::cout<<"X Start: "<<x_start<<std::endl<<std::flush;
		//std::cout<<"X Stop: "<<x_stop<<std::endl<<std::flush;
		//std::cout<<"Y Start: "<<y_start<<std::endl<<std::flush;
		//std::cout<<"Y Stop: "<<y_stop<<std::endl<<std::flush;

		//set the area from the reference frame in the reconstructed frame
		Modify_Image_Range(prev_frame, reconstructed_frame, 0, reconstructed_frame.channels(), x_start, x_stop, y_start, y_stop, macroblock_array[macroblock].block_location_x, macroblock_array[macroblock].block_location_y);
	}
}

//Function to perform the block matching algorithm
//Inputs: Reference Frame, Frame to be Predicted, Reference Frame object to be modified
//Output: True is successful, False if not
bool Block_Match(jbutil::image<int> &frame_1,jbutil::image<int> &frame_2,jbutil::image<int> &reconstructed_frame2)
{
	std::vector<Macroblock> macroblock_array;	//vector to hold all the macroblocks

	if(!(frame_1.get_cols()%block_width == 0))	//checks to ensure that image width and height are exact multiplies of the block width and height
	{
		#ifndef NDEBUG
			  std::cerr << "Error: Block Width and Image Width are not exact multiples \n" << std::flush;
		#endif
		return false;
	}
	else if(!(frame_1.get_rows()%block_height == 0))
	{
		#ifndef NDEBUG
			  std::cerr << "Error: Block Height and Image Height are not exact multiples \n" << std::flush;
		#endif
		return false;
	}

	#ifndef NDEBUG													//Segmenting frame 2 into macroblocks by call to Set_Macroblocks
		  std::cerr << "Segmenting Macroblocks \n" << std::flush;
	#endif
	double t = jbutil::gettime();
	Set_Macroblocks(frame_2, macroblock_array);
	t = jbutil::gettime() - t;
	std::cout << "Time taken for segmentation: " << t << "s" << std::endl;
	#ifndef NDEBUG
	  	  std::cerr << "Macroblocks Successfully Segmented \n" << std::flush;
	#endif

	#ifndef NDEBUG
		  std::cerr << "Starting Block Matching\n" << std::flush;
	#endif
	t = jbutil::gettime();
	for (int macroblock = 0; macroblock <macroblock_array.size(); macroblock++)		//Performing the Block Matching for each macroblock
	{
		Macroblock this_macroblock = macroblock_array[macroblock];

		jbutil::image<int> search_block;						//The search block specific for each macroblock
		float least_MSE = std::numeric_limits<float>::max();	//The lowest MSE found for the macroblock
		int least_MSE_x = this_macroblock.block_location_x;		//The top left column coordinate of the search block with the lowest MSE
		int least_MSE_y = this_macroblock.block_location_y;		//The top left row coordinate of the search block with the lowest MSE
		int new_least_MSE_x = 0;								//These 2 values are temporary values which are updated if a lower MSE search block is
		int new_least_MSE_y = 0;								//found. These are needed since, for a single iteration least_MSE_x and y are constant


		int search_dist_x = search_horizontal/2;				//search dist parameters used in the three step search algorithm
		int search_dist_y = search_vertical/2;

		for (int search_count = 0; search_count<3;search_count++)	//for loop to denote the step in which the 3 step search has reached
		{
			//these 2 for loops are used to define the 9 search blocks for every step in the 3 step search. Note that x and y are used
			//not only as counters but also help to set the search block co-ordinates
			for (int x = -search_dist_x; x<=search_dist_x; x=x+search_dist_x)
			{
				for (int y = -search_dist_y; y<=search_dist_y; y=y+search_dist_y)
				{
					//std::cout<<"X: "<<x<<std::endl<<std::flush;
					//std::cout<<"Y: "<<y<<std::endl<<std::flush;

					//set the search block start and stop co-ordinates

					//Setting the column start and stop
					int x_start = least_MSE_x + x;
					if(x_start < 0)				//check to ensure top left pixel's column coordinate is not less than 0
					{
						continue;
					}

					int x_stop = x_start + block_width;
					if(x_stop > this_macroblock.search_location_x_stop) //check to ensure search area is not out of bounds
					{
						continue;
					}

					//Setting the row start and stop
					int y_start = least_MSE_y + y;
					if(y_start < 0)				//check to ensure top left pixel's row coordinate is not less than 0
					{
						continue;
					}

					int y_stop = y_start + block_height;
					if(y_stop > this_macroblock.search_location_y_stop) //check to ensure search area is not out of bounds
					{
						continue;
					}

					/*
					std::cout<<"COUNT:  "<<search_count<<std::endl<<std::flush;
					std::cout<<"Block x:"<<this_macroblock.block_location_x<<std::endl<<std::flush;
					std::cout<<"Block y:"<<this_macroblock.block_location_y<<std::endl<<std::flush;
					std::cout<<"X-Start "<<x_start<<std::endl<<std::flush;
					std::cout<<"Y-Start "<<y_start<<std::endl<<std::flush;
					std::cout<<"X-Stop  "<<x_stop<<std::endl<<std::flush;
					std::cout<<"Y-Stop  "<<y_stop<<std::endl<<std::flush;*/

					//Set the pixel values for the search block
					Set_Image_Range(frame_1, search_block, 0, frame_1.channels(),x_start, x_stop, y_start, y_stop);


					//Calculate the mse value between the search block and macroblock
				    float current_MSE = MSE(this_macroblock.block,search_block);


					//If a search block with a lower MSE is found, update the parameters
					if(current_MSE<least_MSE)
					{
						least_MSE = current_MSE;
						//Here, cannot use least_MSE_x and least_MSE_y, since these are needed as constants in
						//a single step loop (used when setting co-ordinates for search block)
						new_least_MSE_x = x_start;
						new_least_MSE_y = y_start;

					}
				}
			}

			//Once a step is finished, update these values to represent the block with the lowest MSE
			least_MSE_x = new_least_MSE_x;
			least_MSE_y = new_least_MSE_y;

			//Update also the search dist parameters to get finer searches.
			//After 3 iterations, they should be set to 1 such that macroblocks differ by 1 pixel
			if(search_count == 1)
			{
				search_dist_x = 1;
				search_dist_y = 1;
			}
			else if(search_count != 2)
			{
				//using fast ceil - must be done since no guarantee division will result in exact multiples
				search_dist_x = int((search_dist_x+(search_dist_x/2)-1)/((search_dist_x/2)));
				search_dist_y = int((search_dist_y+(search_dist_y/2)-1)/((search_dist_y/2)));
			}


			//Redefine the new search area, such that it is smaller and centered around the block with the smallest MSE
			//Note that this cannot be done if the count is equal to 2, since this means that there are no more iterations
			if(search_count != 2)
			{
				Set_Search_Area(this_macroblock, frame_1.get_cols(), frame_1.get_rows(), search_dist_y, search_dist_x, least_MSE_x, least_MSE_y);
			}
		}


		//By the final iteration, the least MSE block has been defined as the best MSE macroblock from those searched.
		//therefore the motion vector can be calculated from the top left pixel location of the least mse block and the top left pixel
		//location of the macroblock
		this_macroblock.motion_vector_x = least_MSE_x - this_macroblock.block_location_x;
		this_macroblock.motion_vector_y = least_MSE_y - this_macroblock.block_location_y;

		//std::cout<<"MACROBLOCK: "<<macroblock<<std::endl<<std::flush;
		//std::cout<<"Least MSE: "<<least_MSE<<std::endl<<std::flush;
		//std::cout<<"Motion Vector X: "<<this_macroblock.motion_vector_x<<std::endl<<std::flush;
		//std::cout<<"Motion Vector Y: "<<this_macroblock.motion_vector_y<<std::endl<<std::flush;

		//Update the macroblock in the macroblock array
		macroblock_array[macroblock] = this_macroblock;
	}
	t = jbutil::gettime() - t;
	std::cout << "Time taken for block matching: " << t << "s" << std::endl;
	#ifndef NDEBUG
		  std::cerr << "Block Matching Complete\n" << std::flush;
	#endif

	#ifndef NDEBUG
		  std::cerr << "Reconstructing Frame\n" << std::flush;
	#endif
	t=jbutil::gettime();
	Reconstruct_Frame(frame_1, macroblock_array, reconstructed_frame2);
	t = jbutil::gettime() - t;
	std::cout << "Time taken for reconstruction: " << t << "s" << std::endl;
	#ifndef NDEBUG
		  std::cerr << "Frame Reconstructed Successfully\n" << std::flush;
	#endif

	return true;
}

//Main Function
int main(int argc, char* argv[])
{
	if(argc!=6)
	{
		#ifndef NDEBUG
			  std::cerr << "Not enough input arguments\n" << std::flush;
		#endif
		return 0;
	}

	block_width 		= atoi(argv[1]);
	block_height 		= atoi(argv[2]);
	search_vertical 	= atoi(argv[3]);
	search_horizontal 	= atoi(argv[4]);
	std::string path(argv[5]);

	if((block_width == 0) || (block_height == 0) || (search_vertical == 0) || (search_horizontal == 0))
	{
		std::cerr<<"Integer parameters must be non-zero \n"<<std::flush;
		return 0;
	}


	//Objects to hold the 2 frames
	jbutil::image<int> frame1;
	jbutil::image<int> frame2;

	//Load the 2 frames
	std::ifstream file;
	file.open((path+std::string("/frame1.ppm")).c_str());

	if(file)
	{
		frame1.load(file);
		#ifndef NDEBUG
			  std::cerr << "First Frame Loaded \n" << std::flush;
		#endif
	}
	else
	{
		#ifndef NDEBUG
			  std::cerr << "Error Loading First Frame \n" << std::flush;
		#endif
		return 0;
	}

	file.close();


	file.open((path+std::string("/frame2.ppm")).c_str());
	if(file)
	{
		frame2.load(file);
		#ifndef NDEBUG
			  std::cerr << "Second Frame Loaded \n" << std::flush;
		#endif
	}
	else
	{
		#ifndef NDEBUG
			  std::cerr << "Error Loading Second Frame" << std::flush;
		#endif
		return 0;
	}

	//Object to hold the reconstructed frame 2
	jbutil::image<int> reconstructed_frame2(frame2.get_rows(),frame2.get_cols(),frame2.channels());

	#ifndef NDEBUG
		  std::cerr << "Entering Block Match Function\n" << std::flush;
	#endif
	double t = jbutil::gettime();
	Block_Match(frame1, frame2, reconstructed_frame2);
	t = jbutil::gettime() - t;
	std::cout << "Total Time taken: " << t << "s" << std::endl;
	#ifndef NDEBUG
		  std::cerr << "Exiting Block Match Function\n" << std::flush;
	#endif

	#ifndef NDEBUG
		  std::cerr << "Saving Reconstructed Frame\n" << std::flush;
	#endif
	std::ofstream output;
	output.open((path+std::string("/Reconstructed_Frame.ppm")).c_str());
	reconstructed_frame2.save(output);

	return 0;
}

#include "jbutil.h"
#include <vector>
#include <limits>
#include <istream>
#include <cmath>
#include <string>

//Parameters for the algorithm: macroblock width and height and search area parameters
int block_width = 8;
int block_height = 8;
int search_vertical = 8;
int search_horizontal =8;


bool Load_Frames(std::string path, jbutil::image<int> &frame1,jbutil::image<int> &frame2)
	{
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
				return false;
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
				return false;
			}
			return true;
	}

bool Parameter_Check(jbutil::image<int> &frame_1)
{
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
	return true;
}

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

//Function used to set a start co-ordinate of a search area for a macroblock
//Inputs:  centre_coordinate for the Macroblock whose search area will be set
//		   the distance to be moved to set the output co-ordinate
//Outputs: the start co-ordinate
int Get_Search_Area_Start(int search_dist, int centre_coordinate)
{
	int start = centre_coordinate - search_dist;	//set the top left column/row co-ordinate
	if (start < 0)						//check that it is not out of bounds
	{
		return 0;						//if it is, set it to a default value
	}
	return start;
}

//Function used to set a stop co-ordinate of a search area for a macroblock
//Inputs:  centre_coordinate for the Macroblock whose search area will be set
//		   the distance to be moved to set the output co-ordinate
//		   the maximum row/column value to be used to check for out of bounds
//Outputs: the stop co-ordinate
int Get_Search_Area_Stop(int max,  int search_dist,  int centre_coordinate, int block_distance)
{
	int stop = centre_coordinate + block_distance + search_dist;  //set the final pixel's column co-ordinate
	if(stop > max)					//check that it is not out of bounds
	{
		return max;					//if it is, set it to a default value
	}
	return stop;
}

//Function to perform the block matching algorithm
//Inputs: Reference Frame, Frame to be Predicted, Reference Frame object to be modified
//Output: True is successful, False if not
void Block_Match(jbutil::image<int> &frame_1,jbutil::image<int> &frame_2,jbutil::image<int> &reconstructed_frame2)
{
	//For each macroblock
	for (int macroblock_x = 0; macroblock_x<frame_2.get_cols(); macroblock_x = macroblock_x+block_width)
		{
			for (int macroblock_y = 0; macroblock_y<frame_2.get_rows(); macroblock_y = macroblock_y+block_height)
			{
				//set the search are start and stop co-ordinates
				int search_area_x_start 	= Get_Search_Area_Start(search_horizontal, macroblock_x);
				int search_area_x_stop 		= Get_Search_Area_Stop(frame_1.get_cols(), search_horizontal, macroblock_x, block_width);
				int search_area_y_start 	= Get_Search_Area_Start(search_vertical, macroblock_y);
				int search_area_y_stop 		= Get_Search_Area_Stop(frame_1.get_rows(), search_vertical, macroblock_y, block_height);


				//Set the pixel values for the macroblock
				jbutil::image<int> macroblock;
				Set_Image_Range(frame_2, macroblock, 0, frame_2.channels(), macroblock_x, macroblock_x+block_width, macroblock_y, macroblock_y+block_height);


				jbutil::image<int> search_block;						//The search block specific for each macroblock
				float least_MSE = std::numeric_limits<float>::max();	//The lowest MSE found for the macroblock
				int least_MSE_x = macroblock_x;		//The top left column coordinate of the search block with the lowest MSE
				int least_MSE_y = macroblock_y;		//The top left row coordinate of the search block with the lowest MSE
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

							//set the search block start and stop co-ordinates
							int block_x_start = least_MSE_x + x;
							int block_x_stop = block_x_start + block_width;
							int block_y_start = least_MSE_y + y;
							int block_y_stop = block_y_start + block_height;


							//if out of bounds, skip this iteration
							if((block_x_start < 0)||	(block_x_stop > search_area_x_stop) ||	(block_y_start < 0) || 	(block_y_stop > search_area_y_stop))	//check to ensure top left pixel's column coordinate is not less than 0
							{
								continue;
							}

							//Set the pixel values for the search block
							Set_Image_Range(frame_1, search_block, 0, frame_1.channels(),block_x_start, block_x_stop, block_y_start, block_y_stop);


							//Calculate the mse value between the search block and macroblock
						    float current_MSE = MSE(macroblock,search_block);


							//If a search block with a lower MSE is found, update the parameters
							if(current_MSE<least_MSE)
							{
								least_MSE = current_MSE;
								//Here, cannot use least_MSE_x and least_MSE_y, since these are needed as constants in
								//a single step loop (used when setting co-ordinates for search block)
								new_least_MSE_x = block_x_start;
								new_least_MSE_y = block_y_start;

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
						int search_area_x_start 	= Get_Search_Area_Start(search_dist_x, least_MSE_x);
						int search_area_x_stop 		= Get_Search_Area_Stop(frame_1.get_cols(), search_dist_x, least_MSE_x, block_width);
						int search_area_y_start 	= Get_Search_Area_Start(search_dist_y, least_MSE_y);
						int search_area_y_stop 		= Get_Search_Area_Stop(frame_1.get_rows(), search_dist_y, least_MSE_y, block_height);
					}
				}

				//By the final iteration, the least MSE block has been defined as the best MSE macroblock from those searched.
				//therefore the motion vector can be calculated from the top left pixel location of the least mse block and the top left pixel
				//location of the macroblock
				int motion_vector_x = least_MSE_x - macroblock_x;

				int motion_vector_y = least_MSE_y - macroblock_y;

				//set the area from the reference frame in the reconstructed frame
				int x_start = macroblock_x+motion_vector_x;
				int x_stop = x_start + block_width;

				int y_start = macroblock_y+motion_vector_y;
				int y_stop = y_start + block_height;

				Modify_Image_Range(frame_1, reconstructed_frame2, 0, reconstructed_frame2.channels(), x_start, x_stop, y_start,y_stop, macroblock_x, macroblock_y);

			}

		}
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
		#ifndef NDEBUG
			std::cerr<<"Integer parameters must be non-zero \n"<<std::flush;
		#endif
		return 0;
	}


	//Objects to hold the 2 frames
	jbutil::image<int> frame1;
	jbutil::image<int> frame2;

	//Load the frames
	if(!Load_Frames(path, frame1, frame2))
	{
		return 0;
	}

	//Check the parameters
	if(!Parameter_Check(frame1))
	{
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

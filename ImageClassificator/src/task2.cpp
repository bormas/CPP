#include <string>
#include <vector>
#include <fstream>
#include <cassert>
#include <iostream>
#include <cmath>

#include "classifier.h"
#include "EasyBMP.h"
#include "linear.h"
#include "argvparser.h"
#include "matrix.h"

using std::string;
using std::vector;
using std::ifstream;
using std::ofstream;
using std::pair;
using std::make_pair;
using std::cout;
using std::cerr;
using std::endl;

using std::tuple;
using std::tie;
using std::make_tuple;
using std::get;

using CommandLineProcessing::ArgvParser;

typedef vector<pair<BMP*, int> > TDataSet;
typedef vector<pair<string, int> > TFileList;
typedef vector<pair<vector<float>, int> > TFeatures;

/* ****************BASE*************** */

// Making matrix of brightness
Matrix<float> grayscale(BMP &img) {
	Matrix<float> result(img.TellWidth(), img.TellHeight());
	
	for (unsigned int i = 0; i < result.n_rows; i++) {
		for (unsigned int j = 0; j < result.n_cols; j++) {
			RGBApixel pixel = img.GetPixel(i,j);
			result(i,j) = pixel.Red*0.299 + pixel.Green*0.587 + pixel.Blue*0.114;
		}
	}
	return result;
}

//Horizontal sobel filter
Matrix<float> sobel_x(Matrix<float> &img) {	
	Matrix<float> result(img.n_rows - 2, img.n_cols - 2);
	
	for (unsigned int i = 0; i < img.n_rows - 2; i++)
		for (unsigned int j = 0; j < img.n_cols - 2; j++)
			result(i,j) = img(i + 2, j) - img(i, j);
	
	return result;
}

//Vertical sobel filter
Matrix<float> sobel_y(Matrix<float> &img) {	
	Matrix<float> result(img.n_rows - 2, img.n_cols - 2);
	
	for (unsigned int i = 0; i < img.n_rows - 2; i++)
		for (unsigned int j = 0; j < img.n_cols - 2; j++)
			result(i,j) = img(i, j + 2) - img(i, j);
			
	return result;
}

//Gradient direction, i.e. atan(x/y)
Matrix<float> grad_dir(Matrix<float> &img1, Matrix<float> &img2) {
	
	Matrix<float> result(img1.n_rows, img1.n_cols);
	
	for (unsigned int i = 0; i < result.n_rows; i++) {
		for (unsigned int j = 0; j < result.n_cols; j++) {
			result(i,j) = atan2f(img2(i,j), img1(i,j));
			
			if (std::isnan(result(i,j)))
				result(i,j) = M_PI_2;
			else
				result(i,j) += M_PI;
			
			//checking threshold
			if (result(i,j) < 0)
				result(i,j) = 0;
			if (result(i,j) > 2.0*M_PI)
				result(i,j) = 2.0*M_PI;
		}
	}
	return result;
}

//Gradient's absolute, i.e. sqrt(x^2 + y^2)
Matrix<float> grad_abs(Matrix<float> &img1, Matrix<float> &img2) {
	Matrix<float> result(img1.n_rows, img1.n_cols);
	
	for (unsigned int i = 0; i < result.n_rows; i++)
		for (unsigned int j = 0; j < result.n_cols; j++)
			result(i,j) = sqrt(pow(img1(i,j),2) + pow(img2(i,j),2));
			
	return result;
}

//Normalization of hystogramm (Euclid method)
vector<float> normalization(vector<float> &hystogramm) {
	float sum = 0;
	
	for (unsigned int i = 0; i < hystogramm.size(); ++i)
		sum += pow(hystogramm[i], 2);
	sum = sqrt(sum);
	
	//in case if smt wrong
	if (sum < 1)
		return hystogramm;
		
	for(unsigned int j = 0; j < hystogramm.size(); ++j)
		hystogramm[j] = hystogramm[j]/sum;
	
	return hystogramm;
}

//Calculating hystogramm of oriented gradients
vector<float> oriented_hystogramm(Matrix<float> &g_abs, Matrix<float> &g_dir, unsigned int split, unsigned int div) {
	vector<float> result;
	float size = 2.0*M_PI/split;
	float div_float = static_cast<float>(div);
	
	for (unsigned int m = 0; m < div; ++m) {
		for (unsigned int n = 0; n < div; ++n) {
			vector<float> cell_grad(split);
			
			for (unsigned int p = 0; p < split; ++p)
				cell_grad[p] = 0;
			
			for (unsigned int i = g_abs.n_rows/div_float*m; i < g_abs.n_rows/div_float*(m + 1); ++i) {
				for (unsigned int j = g_abs.n_cols/div_float*n; j < g_abs.n_cols/div_float*(n + 1); ++j) {
					
					unsigned int k = 0;
					//0..2pi choosing index of split
					while (!((g_dir(i,j) >= size*k) && (g_dir(i,j) <= size*(k + 1))))
						k++;
						
					//checking threshold just for sure
					if (k >= split)
						k = split - 1;
					
					//adding absolute value
					cell_grad[k] += g_abs(i,j);
				}
			}
			//normalizing cell_grad vector
			cell_grad = normalization(cell_grad);
			//concatinate to result vector
			result.insert(result.end(), cell_grad.begin(), cell_grad.end());
		}
	}
	return result;
}

/* ****************BASE*************** */




/* ***************TASK_1*************** */

vector<float> LBP(Matrix<float> img, unsigned int div)
{
	vector<float> result;
	vector<float> value(256);				
	int number;	
	float div_float = static_cast<float>(div);
	
	for (unsigned int m = 0; m < div; ++m) {
		for (unsigned int n = 0; n < div; ++n) {
			
			for (unsigned int p = 0; p < value.size(); ++p) 
				value[p] = 0;
			
			//filling value vector	
			for (unsigned int i = img.n_rows/div_float*m + 1; i < img.n_rows/div_float*(m + 1) - 1; ++i) {
				for (unsigned int j = img.n_cols/div_float*n + 1; j < img.n_cols/div_float*(n + 1) - 1; ++j) {
					number = 0;
					
					for(int x = -1; x <= 1; x++) {
						number *= 2;
						if(img(i,j) <= img(i - 1,j + x))
							number++;
					}
					
					number *= 2;
					if(img(i,j) <= img(i,j + 1))
						number++;
					
					for(int x = 1; x >= -1; x--) {
						number *= 2;
						if(img(i,j) <= img(i + 1,j + x))
							number++;
					}
										
					number *= 2;
					if(img(i,j) <= img(i,j - 1))
						number++;
					
					value[number]++;
				}
			}
			//normalizing vector for 1 cell
			value = normalization(value);
			//concatinate to result vector
			result.insert(result.end(), value.begin(), value.end());
		}
	}
	return result;
}

/* ***************TASK_1*************** */




/* ***************TASK_2*************** */

//Calculating color hystogramm
vector<float> color_hystogramm(BMP &img) { 
	
	vector<float> result;
	unsigned long int sum_r, sum_g, sum_b;
	float size_of_cell = img.TellWidth()*img.TellHeight()/64;
	//64 cells
	for (unsigned int m = 0; m < 8; ++m) {
		for (unsigned int n = 0; n < 8; ++n) {
			sum_r = sum_g = sum_b = 0;
			//adding colors in each cell
			for (unsigned int i = img.TellWidth()/8*m; i < img.TellWidth()/8*(m + 1); ++i) {
				for (unsigned int j = img.TellHeight()/8*n; j < img.TellHeight()/8*(n + 1); ++j) {
					RGBApixel pixel = img.GetPixel(i,j);
					sum_r += pixel.Red;
					sum_g += pixel.Green;
					sum_b += pixel.Blue;
				}
			}
			//normalizing colors and adding to result vector
			result.push_back(static_cast<float>(sum_r)/size_of_cell/255);
			result.push_back(static_cast<float>(sum_g)/size_of_cell/255);
			result.push_back(static_cast<float>(sum_b)/size_of_cell/255);
		}
	}
	return result;
}

/* ***************TASK_2*************** */



// Load list of files and its labels from 'data_file' and
// stores it in 'file_list'
void LoadFileList(const string& data_file, TFileList* file_list) {
    ifstream stream(data_file.c_str());

    string filename;
    int label;
    
    int char_idx = data_file.size() - 1;
    for (; char_idx >= 0; --char_idx)
        if (data_file[char_idx] == '/' || data_file[char_idx] == '\\')
            break;
    string data_path = data_file.substr(0,char_idx+1);
    
    while(!stream.eof() && !stream.fail()) {
        stream >> filename >> label;
        if (filename.size())
            file_list->push_back(make_pair(data_path + filename, label));
    }

    stream.close();
}

// Load images by list of files 'file_list' and store them in 'data_set'
void LoadImages(const TFileList& file_list, TDataSet* data_set) {
    for (size_t img_idx = 0; img_idx < file_list.size(); ++img_idx) {
            // Create image
        BMP* image = new BMP();
            // Read image from file
        image->ReadFromFile(file_list[img_idx].first.c_str());
            // Add image and it's label to dataset
        data_set->push_back(make_pair(image, file_list[img_idx].second));
    }
}

// Save result of prediction to file
void SavePredictions(const TFileList& file_list,
                     const TLabels& labels, 
                     const string& prediction_file) {
        // Check that list of files and list of labels has equal size 
    assert(file_list.size() == labels.size());
        // Open 'prediction_file' for writing
    ofstream stream(prediction_file.c_str());

        // Write file names and labels to stream
    for (size_t image_idx = 0; image_idx < file_list.size(); ++image_idx)
        stream << file_list[image_idx].first << " " << labels[image_idx] << endl;
    stream.close();
}

// Exatract features from dataset.
// You should implement this function by yourself =)
void ExtractFeatures(const TDataSet& data_set, TFeatures* features) {
	
	//spliting (0, 2pi) in 'split' parts
	unsigned int split = 16;
	//dividing area in (div x div) cells
	unsigned int div = 8;
	
	//cycle for all images
    for (size_t image_idx = 0; image_idx < data_set.size(); ++image_idx) {
        //base
        Matrix<float> gray_s = grayscale(*data_set[image_idx].first);
		Matrix<float> horizont = sobel_x(gray_s);
		Matrix<float> vertical = sobel_y(gray_s);
		Matrix<float> g_abs = grad_abs(horizont, vertical);
		Matrix<float> g_dir = grad_dir(horizont, vertical);
		vector<float> one_image_features = oriented_hystogramm(g_abs, g_dir, split, div);
		
		//1st task
		vector<float> LBP_hys = LBP(gray_s, div);
		one_image_features.insert(one_image_features.end(), LBP_hys.begin(), LBP_hys.end());
		
		//2nd task
		vector<float> color_hys = color_hystogramm(*data_set[image_idx].first);
		one_image_features.insert(one_image_features.end(), color_hys.begin(), color_hys.end());
		
        features->push_back(make_pair(one_image_features, data_set[image_idx].second));
    }
}

// Clear dataset structure
void ClearDataset(TDataSet* data_set) {
        // Delete all images from dataset
    for (size_t image_idx = 0; image_idx < data_set->size(); ++image_idx)
        delete (*data_set)[image_idx].first;
        // Clear dataset
    data_set->clear();
}

// Train SVM classifier using data from 'data_file' and save trained model
// to 'model_file'
void TrainClassifier(const string& data_file, const string& model_file) {
        // List of image file names and its labels
    TFileList file_list;
        // Structure of images and its labels
    TDataSet data_set;
        // Structure of features of images and its labels
    TFeatures features;
        // Model which would be trained
    TModel model;
        // Parameters of classifier
    TClassifierParams params;
    
        // Load list of image file names and its labels
    LoadFileList(data_file, &file_list);
        // Load images
    LoadImages(file_list, &data_set);
        // Extract features from images
    ExtractFeatures(data_set, &features);

        // PLACE YOUR CODE HERE
        // You can change parameters of classifier here
    params.C = 0.01;
    TClassifier classifier(params);
        // Train classifier
    classifier.Train(features, &model);
        // Save model to file
    model.Save(model_file);
        // Clear dataset structure
    ClearDataset(&data_set);
}

// Predict data from 'data_file' using model from 'model_file' and
// save predictions to 'prediction_file'
void PredictData(const string& data_file,
                 const string& model_file,
                 const string& prediction_file) {
        // List of image file names and its labels
    TFileList file_list;
        // Structure of images and its labels
    TDataSet data_set;
        // Structure of features of images and its labels
    TFeatures features;
        // List of image labels
    TLabels labels;

        // Load list of image file names and its labels
    LoadFileList(data_file, &file_list);
        // Load images
    LoadImages(file_list, &data_set);
        // Extract features from images
    ExtractFeatures(data_set, &features);

        // Classifier 
    TClassifier classifier = TClassifier(TClassifierParams());
        // Trained model
    TModel model;
        // Load model from file
    model.Load(model_file);
        // Predict images by its features using 'model' and store predictions
        // to 'labels'
    classifier.Predict(features, model, &labels);

        // Save predictions
    SavePredictions(file_list, labels, prediction_file);
        // Clear dataset structure
    ClearDataset(&data_set);
}

int main(int argc, char** argv) {
    // Command line options parser
    ArgvParser cmd;
        // Description of program
    cmd.setIntroductoryDescription("Machine graphics course, task 2. CMC MSU, 2014.");
        // Add help option
    cmd.setHelpOption("h", "help", "Print this help message");
        // Add other options
    cmd.defineOption("data_set", "File with dataset",
        ArgvParser::OptionRequiresValue | ArgvParser::OptionRequired);
    cmd.defineOption("model", "Path to file to save or load model",
        ArgvParser::OptionRequiresValue | ArgvParser::OptionRequired);
    cmd.defineOption("predicted_labels", "Path to file to save prediction results",
        ArgvParser::OptionRequiresValue);
    cmd.defineOption("train", "Train classifier");
    cmd.defineOption("predict", "Predict dataset");
        
        // Add options aliases
    cmd.defineOptionAlternative("data_set", "d");
    cmd.defineOptionAlternative("model", "m");
    cmd.defineOptionAlternative("predicted_labels", "l");
    cmd.defineOptionAlternative("train", "t");
    cmd.defineOptionAlternative("predict", "p");

        // Parse options
    int result = cmd.parse(argc, argv);

        // Check for errors or help option
    if (result) {
        cout << cmd.parseErrorDescription(result) << endl;
        return result;
    }

        // Get values 
    string data_file = cmd.optionValue("data_set");
    string model_file = cmd.optionValue("model");
    bool train = cmd.foundOption("train");
    bool predict = cmd.foundOption("predict");

        // If we need to train classifier
    if (train)
        TrainClassifier(data_file, model_file);
        // If we need to predict data
    if (predict) {
            // You must declare file to save images
        if (!cmd.foundOption("predicted_labels")) {
            cerr << "Error! Option --predicted_labels not found!" << endl;
            return 1;
        }
            // File to save predictions
        string prediction_file = cmd.optionValue("predicted_labels");
            // Predict data
        PredictData(data_file, model_file, prediction_file);
    }
}

#pragma once

/** @file cuvis_spectral.hpp
  *
  * 
  * @authors BenMGeo, Simon
  * @date 02/08/2023
  * @details written for use with cuvis measurements of all modes. 
  * @copyright Apache V2.0
  * */


#include <cassert>
#include <cuvis.hpp>
#include <numeric>
#include <opencv2/opencv.hpp>
#include <vector>

/**
  * @brief Helper functions and structures for spectral calculation.
  *
  * Spectral helpers include structures and methods to extract spectra of type spectrum_t
  * from polygones of type polygon_t and calculate wavelength specific histograms of type histogram_vector_t.
  * */
namespace cuvis::aux::spectral
{

  template <typename data_t>
  inline int get_mat_datatype(int channel_count)
  {
    if (channel_count < 1 || channel_count > 511)
      throw std::invalid_argument("Invalid channel count");

    if constexpr (!std::is_floating_point<data_t>::value)
    {
      if constexpr (!std::is_signed<data_t>::value)
      {
        switch (sizeof(data_t))
        {
          case 1: return CV_MAKETYPE(CV_8U, channel_count);
          case 2: return CV_MAKETYPE(CV_16U, channel_count);
          default: throw std::invalid_argument("Invalid bitdepth for unsigned integer data type");
        }
      }
      else
      {
        switch (sizeof(data_t))
        {
          case 1: return CV_MAKETYPE(CV_8S, channel_count);
          case 2: return CV_MAKETYPE(CV_16S, channel_count);
          case 4: return CV_MAKETYPE(CV_32S, channel_count);
          default: throw std::invalid_argument("Invalid bitdepth for signed integer data type");
        }
      }
    }
    else
    {
      switch (sizeof(data_t))
      {
        case 2: return CV_MAKETYPE(CV_16F, channel_count);
        case 4: return CV_MAKETYPE(CV_32F, channel_count);
        case 8: return CV_MAKETYPE(CV_64F, channel_count);
        default: throw std::invalid_argument("Invalid bitdepth for floating point data type");
      }
    }
  }

  /** @brief Couple of wavelength, respective mean value and standard deviation with default values.
   *  
   * Basic type for spectral information.
   * */
  struct spectral_mean_t
  {
    /** The wavelength (in nm).*/
    std::uint32_t wavelength;

    /** The value (counts/reflectance, depending on input).*/
    double value = //default value:
        -999.0;

    /** The standard deviation for the repsective value.*/
    double std = //default value:
        0.0;
  };

  /** @brief Couple of wavelength, respective count and occurrence
   */
  struct histogram_t
  {
    /** The center wavelength (in nm).*/
    std::uint32_t wavelength;

    /** The count for a specific center wavelength.*/
    std::vector<std::float_t> count;

    /** The occurence for the respective count.*/
    std::vector<std::uint64_t> occurrence;
  };

  /** @brief A vector type for describing a spectrum with mean and standard deviation
   * */
  using spectrum_t = std::vector<spectral_mean_t>;

  /** @brief A vector type for describing a histogram for individual wavelengths with counts and ocurrences
   * */
  using histogram_vector_t = std::vector<histogram_t>;

  /** @brief 2-dimensional definition of a single point
   * */
  struct point_t
  {
    /** The x coordinate. (E-W)*/
    double _x;

    /** The y coordinate. (S-N)*/
    double _y;
  };

  /** @brief A vector type for describing a polygon with x and y coordinates
   * */
  using polygon_t = std::vector<point_t>;

  template <typename data_t>

  /** @brief Calculates a spectrum for a polygon.
    *
    * Calculates a spectrum with mean and standard deviation for all 
    * wavelengths for a given polygon, i.e. vector of points.
    *  
    * @param[in] img Cuvis Image data from @ref Measurement
    * @param[in] poly Polygon for subsetting the image (can also be only 1 point)
    * @returns A vector of type @ref spectrum_t
    * */
  spectrum_t get_spectrum_polygon(image_t<data_t> const& img, polygon_t const& poly)
  {
    //checks if image is reasonable.
    assert(img._width > 1);
    assert(img._height > 1);
    assert(img._channels > 0);
    assert(img._wavelength != nullptr);

    // initializing the result
    spectrum_t res(img._channels);

    // conversion of polygon relative coordinates to absolute pixel coordinates
    if (poly.size() > 1) //polygon case
    {

      std::vector<cv::Point> transformed_pt; //needs to be integers
      for (auto const& pt : poly)
      {
        transformed_pt.emplace_back(pt._x * (img._width - 1), pt._y * (img._height - 1));
      }

      //empty mask
      cv::Mat mask = cv::Mat::zeros(cv::Size(img._width, img._height), CV_8UC1);

      // https://docs.opencv.org/4.2.0/db/d75/samples_2cpp_2create_mask_8cpp-example.html#a16
      // apparently this now has to be wrapped

      std::vector<std::vector<cv::Point>> vpts;
      vpts.push_back(transformed_pt);

      //binary mask
      //using nearest neighbor
      cv::fillPoly(mask, vpts, cv::Scalar(255));

      std::uint64_t n = 0;                                     //counter variable
      std::vector<std::double_t> sum_v(img._channels, 0.0);    //growing sum
      std::vector<std::double_t> sq_sum_v(img._channels, 0.0); //growing square sum

      //checking all pixels in polygon mask
      for (int x = 0; x < img._width; x++)
      {
        for (int y = 0; y < img._height; y++)
        {
          if (mask.at<std::uint8_t>(y, x) > 128)
          {
            n++;
            for (int z = 0; z < img._channels; z++)
            {
              //reading out value
              auto loc_val = img._data[((y) * img._width + (x)) * img._channels + (z)];
              //adding up values
              sum_v[z] += loc_val;
              sq_sum_v[z] += loc_val * loc_val;
            }
          }
        }
      }

      for (int z = 0; z < img._channels; z++)
      {
        // convert sum to mean
        double mean = sum_v[z] / n;
        spectral_mean_t loc_res;
        loc_res.value = mean;

        // sum(x - y)^2 = sum( x^2 - 2xy + y^2) // second binomial formula
        // Therefore: (sum(x^2) - 2 * sum(x) * y + n*y^2 ) / n
        // Therefore (sum(x^2) - 2 * sum(x) * y ) / n + y^2
        loc_res.std = sqrt((sq_sum_v[z] - 2 * sum_v[z] * mean) / n + mean * mean);

        // set wavelength
        std::uint32_t loc_wl = img._wavelength[z];
        loc_res.wavelength = loc_wl;
        res[z] = loc_res;
      }

      return res;
    }
    else if (poly.size() == 1) //single point case
    {
      //check if out of range
      if (poly.front()._x > 1.0 || poly.front()._x < 0.0 || poly.front()._y > 1.0 || poly.front()._y < 0.0)
      {
        // outside range. nothing to return
        // TODO throw error?
        return res;
      }

      for (int z = 0; z < img._channels; z++)
      {
        //calculate position in memory
        int y_shift = std::round(poly.front()._y * (img._height - 1)) * img._width;
        int xy_shift = y_shift + std::round(poly.front()._x * (img._width - 1));
        //read out point value
        double loc_val = img._data[xy_shift * img._channels + z];
        //set wavelength and value
        std::uint32_t loc_wl = img._wavelength[z];
        spectral_mean_t loc_res;
        loc_res.value = loc_val;
        loc_res.wavelength = loc_wl;
        res[z] = (loc_res);
      }
      return res;
    }
    else // poly is empty should never happen
    {
      // TODO throw error?
      return res;
    }
  }

  /** @brief Calculates a histogram for an image
    *
    * Calculates a histogram for all wavelengths with counts and occurence
    * for a given Cuvis @ref Measurement Image.
    *  
    * @param[in] img Cuvis Image data from @ref Measurement
    * @param[in] histogram_min_size Lower limit for image data points
    * @param[in] count_bins Number of count bins of histogram
    * @param[in] wavelength_bins Number of wavelength bins of histogram
    * @returns A vector of type @ref histogram_vector_t
    * */
  template <typename data_t>
  histogram_vector_t get_histogram(image_t<data_t> const& img, size_t histogram_min_size, size_t count_bins, size_t wavelength_bins, bool detect_max_value, cuvis_processing_mode_t proc_mode)
  {
    histogram_vector_t output;
    // Check if data is available and that the image is large enough
    assert(img._height * img._width * img._channels > histogram_min_size);
    assert(img._wavelength != nullptr);

    auto mat_datatype = get_mat_datatype<data_t>(img._channels);

    // Yes, the 'const' needs to be removed, OpenCV does not have a cv::ConstMat
    // Making the Mat itself const only means that the Mat header (meta-data) is const.
    // ! Make sure that data in this Mat is not modified in this function !
    const cv::Mat const_img_mat(cv::Size(img._width, img._height), mat_datatype, const_cast<void*>(static_cast<const void*>(img._data)));

    // Find maximum value for all data
    double max_val = 0;
    if (detect_max_value)
    {
      cv::minMaxLoc(const_img_mat, nullptr, &max_val, nullptr, nullptr);
    }
    else
    {
      max_val = double(std::numeric_limits<data_t>::max());
    }

    const double bin_size = max_val / double(count_bins);
    const double wlbin_size = double(img._channels) / double(wavelength_bins);
    const int img_channels_per_wlbin = static_cast<const int>(wlbin_size);
    const int histogram_count = img._channels / img_channels_per_wlbin;

    // Calculate histograms
    for (int count = 0; count < histogram_count; count++)
    {
      int bins[] = {int(count_bins)};
      float range[] = {0.0f, float(max_val)};
      const float* ranges[] = {range};
      cv::Mat hist;

      int channels[] = {0};
      // Accumulate histograms per channel
      for (int c = 0; c < img_channels_per_wlbin; c++)
      {
        channels[0] = c + count * img_channels_per_wlbin;
        bool accumulate = c != 0;
        cv::calcHist(&const_img_mat, 1, channels, cv::Mat(), hist, 1, bins, ranges, true, accumulate);
      }

      // Prepare histogram data struct
      histogram_t histogram;
      histogram.occurrence = std::vector<std::uint64_t>(hist.total());
      histogram.count = std::vector<std::float_t>(hist.total());
      histogram.wavelength = img._wavelength[count * img_channels_per_wlbin + int(img_channels_per_wlbin / 2)];

      // Transfer histogram counts & set x-axis labels
      for (int idx = 0; idx < hist.total(); idx++)
      {
        // Note: calcHist always returns counts as floats
        histogram.occurrence[idx] = static_cast<std::uint64_t>(reinterpret_cast<float*>(hist.data)[idx]);
        if (proc_mode == Cube_Reflectance)
        {
          histogram.count[idx] = static_cast<std::float_t>((idx * bin_size) / 100.0);
        }
        else
        {
          histogram.count[idx] = static_cast<std::float_t>(idx * bin_size);
        }
      }

      output.push_back(histogram);
    }
    return output;
  }

} // namespace cuvis::aux::spectral
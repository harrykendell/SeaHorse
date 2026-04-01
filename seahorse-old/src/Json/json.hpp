#include "src/Json/nlohmann_json.hpp"
#include "src/Physics/Vectors.hpp"
#include "src/Utils/Logger.hpp"

using json = nlohmann::json;

void save(const std::string& filename, const json& j)
{
    std::string tmp_filename = filename + ".tmp";
    // check if we can open the file
    std::ofstream o(tmp_filename); // Write to a temporary file
    if (!o.is_open()) {
        S_FATAL("Could not open file: ", filename);
    }
    o << j.dump(4) << std::endl;
    o.close();
    std::rename(tmp_filename.c_str(), filename.c_str());
}

json load(const std::string& filename)
{
    std::ifstream i(filename);
    if (!i.is_open()) {
        S_FATAL("Could not open file: ", filename);
    }
    json j;
    i >> j;
    i.close();
    return j;
}

namespace std {

template <class T>
void to_json(json& j, const std::complex<T>& p)
{
    j = json { p.real(), p.imag() };
}

template <class T>
void from_json(const json& j, std::complex<T>& p)
{
    p.real(j.at(0));
    p.imag(j.at(1));
}
} // namespace std

namespace Eigen {
// All types like RVec, CVec, RMat, CMat
template <typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
void to_json(nlohmann::json& j, const Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& matrix)
{
    for (Eigen::Index r = 0; r < matrix.rows(); ++r) {
        if (matrix.cols() > 1) {
            nlohmann::json jrow = nlohmann::json::array();
            for (Eigen::Index c = 0; c < matrix.cols(); ++c) {
                jrow.push_back(matrix(r, c));
            }
            j.emplace_back(std::move(jrow));
        } else {
            j.push_back(matrix(r, 0));
        }
    }
}

// CVecs - specialised to avoid .at() calls on a number
void from_json(const json& j, CVec& v)
{
    std::vector<std::complex<double>> v1 = j.get<std::vector<std::complex<double>>>();
    v = Eigen::Map<CVec, Eigen::Unaligned>(v1.data(), v1.size());
}

// RVec/RMats/CMats
template <typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
void from_json(const nlohmann::json& j, Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& matrix)
{
    bool resized = false;
    auto resize = [&](size_t nrows, size_t ncols) {
        if (!resized) {
            matrix.resize(nrows, ncols);
            resized = true;
        } else if (matrix.rows() != nrows || matrix.cols() != ncols) {
            S_FATAL("The number of rows/cols changed inside the Vector/Matrix!");
        }
    };

    for (size_t r = 0, nrows = j.size(); r < nrows; ++r) {
        const auto& jrow = j.at(r);
        if (jrow.is_array()) {
            const size_t ncols = jrow.size();
            resize(nrows, ncols);
            for (size_t c = 0; c < ncols; ++c) {
                const auto& value = jrow.at(c);
                matrix(static_cast<Index>(r), static_cast<Index>(c)) = value.get<_Scalar>();
            }
        } else {
            resize(nrows, 1);
            matrix(static_cast<Index>(r), 0) = jrow.get<_Scalar>();
        }
    }
}

} // namespace Eigen
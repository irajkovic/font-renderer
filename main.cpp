/**
  *
  * Generates font bitmap array that can be used to render text by copying the
  * needed character bitmaps.
  *
  * Usage:
  * $ ./FontRenderer [ascii-from] [ascii-to] [C++ type] [array-name]
  *                  [ [[font-name] [font-size ..] .. ]
  *
  * Example:  *
  * $ ./FontRenderer 33 127 Arial 12 18 Consolas 32
  * Generates Arial bitmaps in sizes 12 and 18 and Consolas bitmaps in size 32
  * for ascii characters 33 to 127 (decimal).
  *
  */

#include <QGuiApplication>
#include <QPixmap>
#include <QPainter>
#include <QString>
#include <QFont>
#include <QFontMetrics>
#include <iostream>
#include <cstdlib>
#include <tuple>

struct FontDesc
{
    std::string name;
    int size;
    uint8_t from;
    uint8_t to;
};

/**
 * @brief getCharacters creates a string of sequential ascii characters.
 *
 * @param from  First character in a sequence.
 * @param to    Last character in a sequence.
 *
 * @return the resulting string (closed interval from..to).
 *
 */
QString getCharacters(uint8_t from, uint8_t to)
{
    QString result;
    for (uint8_t c = from ; c <= to ; ++c) {
        result += c;
    }

    return result;
}

/**
 * @brief getMetrics returns font metrics object used to get the font geometry
 * details.
 *
 * @param font The QFont object.
 *
 * @return QFontMetrics object.
 */
QFontMetrics getMetrics(QFont font)
{
    QPixmap pixmap(1, 1);
    QPainter painter(&pixmap);
    painter.setFont(font);
    return painter.fontMetrics();
}

/**
 * @brief normalize returns a single number representing the color intensity of
 * a QRgb pixel, normalized to range 0..ceiling.
 *
 * @param color Color in QRgb
 * @param ceiling Value to normalize to
 * @return Value in a range 0..ceiling
 */
int normalize(QRgb color, int ceiling)
{
    uint8_t red = (color & 0x00FF0000) >> 16;
    uint8_t green = (color & 0x0000FF00) >> 8;
    uint8_t blue = color & 0x000000FF;
    return (red + green + blue) * ceiling / (3 * 0xFF);
}

/**
 * @brief indent returns indentation string.
 * @param tabs Number of \t characters to return.
 * @return Tab ascii symbol repeated "tabs" times.
 */
std::string indent(int tabs)
{
    return std::string(tabs, '\t');
}

/**
 * @brief openFontArray prints the opening array definition.
 * @param type type of the array element
 * @param name name of the array
 */
void openFontArray(const std::string &type,
                   const std::string &name)
{
    std::cout << type << " " << name << " = " << std::endl;
    std::cout << "{" << std::endl;
}

/**
 * @brief closeFontArray prints the array closing sequence.
 */
void closeFontArray()
{
    std::cout << "};" << std::endl;
}

/**
 * @brief appendFontBitmap renders ascii characters using the given font and
 * prints them in the array format.
 * @param characters String containing the characters sequence.
 * @param font Font object.
 * @param desc Additional font description.
 */
void appendFontBitmap(QString characters, QFont font, FontDesc desc)
{
    QFontMetrics metrics = getMetrics(font);
    QPixmap pixmap(metrics.maxWidth(), metrics.height());

    QPainter painter(&pixmap);
    painter.setFont(font);
    painter.setPen(Qt::white);

    int height = metrics.height();
    int y = metrics.overlinePos() - 1;

    std::cout << indent(1) << "{" << std::endl
              << indent(2) << "\"" << desc.name << "\"," << std::endl
              << indent(2) << desc.size << "," <<  std::endl
              << indent(2) << height << "," << std::endl
              << indent(2) << +desc.from << "," << std::endl
              << indent(2) << +desc.to << "," << std::endl
              << indent(2) << "{" << std::endl;

    for (const auto& c : characters) {

        pixmap.fill(Qt::black);
        painter.drawText(0, y, c);
        QImage image = pixmap.toImage();

        int width = metrics.width(c);

        std::cout << indent(3) << "{" << std::endl;
        std::cout << indent(4) << width << "," << std::endl;
        std::cout << indent(4) << "{" << std::endl;

        for (int y = 0 ; y < height ; ++y) {
            std::cout << indent(5);
            for (int x = 0 ; x < width ; ++x) {
                std::cout << normalize(image.pixel(x, y), 255) << ",";
            }
            std::cout << std::endl;
        }

        std::cout << indent(4) << "}" << std::endl;
        std::cout << indent(3) << "}," << std::endl;
    }

    std::cout << indent(2) << "}" << std::endl;
    std::cout << indent(1) << "}" << std::endl;
}

/**
 * @brief main Renders font bitmaps into the uint8_t arrays, allowing C / C++
 * program to print the text by copying the bytes directly to the video output
 * buffer.
 * @param argc Number of arguments.
 * @param argv List of string arguments.
 * @retval EXIT_SUCCESS on success
 * @retval EXIT_FAILURE if arguments are not properly formated.
 */
int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);

    // TODO :: Improve agrument parsing with QCommandLineParser.

    if (argc < 7) {
        std::cout << "Basic usage: " << argv[0] << " [ascii-from] [ascii-to] "
                  << "[cpp-type] [arr-name] [[font-name] [font-size] ..]"
                  << std::endl;
        return EXIT_FAILURE;
    }

    int arg_ind = 1;    // Skip program name
    auto from = std::stoul(argv[arg_ind++]);
    auto to = std::stoul(argv[arg_ind++]);

    FontDesc desc;
    desc.from = from <= 255 ? static_cast<uint8_t>(from) : 0;
    desc.to = to <= 255 ? static_cast<uint8_t>(to) : 0;

    if (desc.from > desc.to) {
        return EXIT_FAILURE;
    }

    QString characters = getCharacters(desc.from, desc.to);

    auto type = argv[arg_ind++];
    auto name = argv[arg_ind++];
    openFontArray(type, name);

    while (arg_ind < argc) {

        try {
            auto number = std::stoul(argv[arg_ind]);
            desc.size = number;

            if (desc.name.empty() == false) {
                QFont font(QString::fromStdString(desc.name), desc.size);
                appendFontBitmap(characters, font, desc);
            }
        }
        catch(std::invalid_argument &err) {
            (void)err;
            desc.name = argv[arg_ind];
        }

        ++arg_ind;
    }

    closeFontArray();

    return EXIT_SUCCESS;
}

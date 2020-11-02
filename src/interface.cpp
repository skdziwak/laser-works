#include <math.h>
#include <fstream>
#include <string>

#include "interface.h"
#include "resources.h"
#include "utils.h"

using namespace std;

const string Interface::windowName = "LaserWorks";

Interface::Interface() {
    this->paths = NULL;

    int argc = 0;
    char **argv = NULL;

    this->app = Gtk::Application::create(argc, argv, "com.skddev.laserworks");
    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_string(res::GLADE_MAIN);
    
    builder->get_widget("root", this->window);
    builder->get_widget("drawing_area", this->drawingArea);
    builder->get_widget("load_svg_button", this->loadSvgButton);
    builder->get_widget("export_gcode_button", this->exportGcodeButton);
    builder->get_widget("tree_view", this->propertiesTreeView);
    builder->get_widget("start_gcode_text_view", this->startGcodeTextView);
    builder->get_widget("end_gcode_text_view", this->endGcodeTextView);
    builder->get_widget("tool_on_gcode_text_view", this->toolOnGcodeTextView);
    builder->get_widget("tool_off_gcode_text_view", this->toolOffGcodeTextView);
    builder->get_widget("load_svg_menu", this->loadSvgMenuItem);
    builder->get_widget("export_gcode_menu", this->exportGcodeMenuItem);
    builder->get_widget("exit_menu", this->exitMenuItem);
    builder->get_widget("about_menu", this->aboutMenuItem);

    this->loadSvgButton->signal_clicked()
                .connect(sigc::mem_fun(*this, &Interface::loadSvgButtonClicked));
    this->exportGcodeButton->signal_clicked()
                .connect(sigc::mem_fun(*this, &Interface::exportGcodeButtonClicked));
    this->loadSvgMenuItem->signal_activate()
                .connect(sigc::mem_fun(*this, &Interface::loadSvgButtonClicked));
    this->exportGcodeMenuItem->signal_activate()
                .connect(sigc::mem_fun(*this, &Interface::exportGcodeButtonClicked));
    this->exitMenuItem->signal_activate()
                .connect(sigc::mem_fun(*this->window, &Gtk::Window::close));
    this->drawingArea->signal_draw().connect(sigc::mem_fun(*this, &Interface::draw));

    this->refPropertiesListStore = Gtk::ListStore::create(this->propertiesModel);
    this->propertiesTreeView->set_model(this->refPropertiesListStore);

    this->refPropertiesListStore->signal_row_changed().connect(sigc::mem_fun(*this, &Interface::requestDraw));

    this->propertiesTreeView->append_column("Property", this->propertiesModel.m_col_property);
    this->propertiesTreeView->append_column_numeric_editable("Value", this->propertiesModel.m_col_value, "%.0f");

    if(!loadConfig()) {
        //Default values
        this->rowOffsetX = this->addProperty("Offset X (mm)", 0);
        this->rowOffsetY = this->addProperty("Offset Y (mm)", 0);
        this->rowBedWidth = this->addProperty("Bed width (mm)", 220);
        this->rowBedHeight = this->addProperty("Bed height (mm)", 220);
        this->rowTravelSpeed = this->addProperty("Travel speed (mm/s)", 800);
        this->rowWorkingSpeed = this->addProperty("Working speed (mm/s)", 400);
    }

    //Loading CSS
    auto css = Gtk::CssProvider::create();
    if(not css->load_from_data(res::CSS_STYLE)) {
        printf("Failed to load css.\n");
    } else {
        auto screen = Gdk::Screen::get_default();
        auto ctx = this->window->get_style_context();
        ctx->add_provider_for_screen(screen, css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    //Loading an icon
    unsigned char* data = reinterpret_cast<unsigned char*>(const_cast<char*>(res::ICON));
    auto stream = Gio::MemoryInputStream::create();
    stream->add_bytes(Glib::Bytes::create(res::ICON, res::ICON_LENGTH));
    this->icon = Gdk::Pixbuf::create_from_stream(stream);

    this->window->set_icon(this->icon);
    this->window->set_title(Interface::windowName);
}

Interface::~Interface() {
    delete this->window;
    delete this->paths;
}

void Interface::run() {
    this->window->show();
    this->app->run(*this->window);
}

void Interface::loadSvgButtonClicked() {
    Gtk::FileChooserDialog dialog("Choose an SVG file.", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Open", Gtk::RESPONSE_OK);
    auto filter = Gtk::FileFilter::create();
    filter->set_name("SVG files");
    filter->add_pattern("*.svg");
    dialog.add_filter(filter);

    int result = dialog.run();

    if(result == Gtk::RESPONSE_OK) {
        string path = dialog.get_filename();
        try {
            delete this->paths;

            int index = path.rfind('/');
            if(index == -1) index = path.rfind('\\');
            if(index == -1) index = 0;
            string name = path.substr(index + 1);
            this->window->set_title(Interface::windowName + " - " + name);

            this->paths = svg::loadPaths(path);

        } catch(exception const &e) { //Catching all exceptions and showing error to user
            Gtk::MessageDialog messageDialog(*this->window, "Loading SVG file failed.", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
            messageDialog.set_secondary_text(string(e.what()));
            messageDialog.set_icon(this->icon);
            messageDialog.set_title("Error");
            messageDialog.run();
        }
    }
}

bool has_suffix(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void Interface::exportGcodeButtonClicked() {
    Gtk::FileChooserDialog dialog("Save GCODE file.", Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Save", Gtk::RESPONSE_OK);
    auto filter = Gtk::FileFilter::create();
    filter->set_name("GCODE files");
    filter->add_pattern("*.gcode");
    dialog.add_filter(filter);

    int result = dialog.run();
    if(result == Gtk::RESPONSE_OK) {
        const double step_size = 0.01;
        const double acceptable_gap = 0.07;

        string path = dialog.get_filename();
        string startGcode = this->startGcodeTextView->get_buffer()->get_text();
        string endGcode = this->endGcodeTextView->get_buffer()->get_text();
        string toolOnGcode = this->toolOnGcodeTextView->get_buffer()->get_text();
        string toolOffGcode = this->toolOffGcodeTextView->get_buffer()->get_text();
        
        if(!has_suffix(path, ".gcode")) path += ".gcode";
        
        fstream file;
        file.open(path, ios::out);
        file <<  "G21 ; Metric system\n";
        file <<  "G90 ; Absolute positioning\n";
        file <<  "G28 ; Home all axes\n";
        file << startGcode << "\n";
        file << toolOffGcode << "\n";
        double offsetX = this->getRowValue(this->rowOffsetX);
        double offsetY = this->getRowValue(this->rowOffsetY);
        double bed_height = this->getRowValue(this->rowBedHeight);
        double travelSpeed = this->getRowValue(this->rowTravelSpeed);
        double workingSpeed = this->getRowValue(this->rowWorkingSpeed);
        if(bed_height < 10) bed_height = 10;

        bool toolEnabled = false;
        svg::Point lastPoint = {0, 0};
        for(svg::Path path : *this->paths) {
            svg::Transformation transformation = path.getTransformation();
            for(svg::PathElement *element : path) {
                svg::Point start = element->getPoint(0) * transformation;
                if(abs(lastPoint.x - start.x) > acceptable_gap || abs(lastPoint.y - start.y) > acceptable_gap) {
                    if(toolEnabled) {
                        file << toolOffGcode << "\n";
                        toolEnabled = false;
                    }
                    file << "M203 X" << travelSpeed << " Y" << travelSpeed << "\n";
                    file << "G1 X" << (start.x + offsetX) << " Y" << (bed_height - (start.y - offsetY)) << "\n";
                }
                if(!toolEnabled) {
                    file << toolOnGcode << "\n";
                    toolEnabled = true;
                }
                file << "M203 X" << workingSpeed << " Y" << workingSpeed << "\n";
                for(double d = 0 ; d < 1 + step_size ; d += step_size) {
                    svg::Point p = element->getPoint(d) * transformation;
                    file << "G1 X" << (p.x + offsetX) << " Y" << (bed_height - (p.y - offsetY)) << "\n";
                }
                lastPoint = element->getPoint(1) * transformation;
            }
        }


        file << endGcode;
        file.close();
        if(!file) {
            Gtk::MessageDialog messageDialog(*this->window, "Exporting GCODE file failed.", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
            messageDialog.set_icon(this->icon);
            messageDialog.set_secondary_text("Output error");
            messageDialog.set_title("Error");
            messageDialog.run();
        }
    }

}

bool Interface::draw(const Cairo::RefPtr<Cairo::Context> &ctx) {
    Gtk::Allocation allocation = this->drawingArea->get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();
    //Smaller canvas direction
    double min_size = min(width, height);

    double bed_width = this->getRowValue(this->rowBedWidth);
    double bed_height = this->getRowValue(this->rowBedHeight);
    double offset_x = this->getRowValue(this->rowOffsetX);
    double offset_y = this->getRowValue(this->rowOffsetY);
    if(bed_width < 10) bed_width = 10;
    if(bed_height < 10) bed_height = 10;

    //Bigger bed dimension
    double bed_max_size = max(bed_width, bed_height);

    double zoom = 0.9;
    double bed_scale = min_size / bed_max_size * zoom;

    //Location of bed (0, 0) position on canvas
    double bedX = width / 2 - bed_width * bed_scale / 2;
    double bedY = height / 2 - bed_height * bed_scale / 2;
    

    int grid_number = 2 * (min_size / 50);
    double grid_size = bed_max_size * bed_scale / grid_number;
    
    ctx->save();

    //Background
    ctx->set_source_rgb(0.95, 0.95, 0.95);
    ctx->paint();

    //Grid
    ctx->set_source_rgb(0.3, 0.3, 0.3);
    for(int x = -(bedX / grid_size + 1) ; x < ceil(grid_number + 2 * bedX / grid_size + 1) ; x++) {
        ctx->set_line_width(x % 2 == 0 ? 1.0 : 0.5);
        ctx->move_to(x * grid_size + bedX, 0);
        ctx->line_to(x * grid_size + bedX, height);
        ctx->stroke();
    }
    for(int y = -(bedY / grid_size + 1) ; y < ceil(grid_number + 2 * bedY / grid_size + 1) ; y++) {
        ctx->set_line_width(y % 2 == 0 ? 1.0 : 0.5);
        ctx->move_to(0, y * grid_size  + bedY);
        ctx->line_to(width, y * grid_size + bedY);
        ctx->stroke();
    }

    //Bed
    ctx->set_source_rgb(0.1, 0.1, 0.7);
    ctx->set_line_width(2.0);
    ctx->rectangle(bedX, bedY, bed_width * bed_scale, bed_height * bed_scale);
    ctx->stroke();

    //Offsets
    ctx->set_source_rgb(0.1, 0.7, 0.1);
    ctx->set_line_width(2.0);
    if(offset_x != 0) {
        double x = bedX + offset_x * bed_scale;
        ctx->move_to(x, 0);
        ctx->line_to(x, height);
        ctx->stroke();
    }
    if(offset_y != 0) {
        double y = bedY + (bed_height - offset_y) * bed_scale;
        ctx->move_to(0, y);
        ctx->line_to(width, y);
        ctx->stroke();
    }

    //Draw paths
    ctx->set_line_width(1.0);
    ctx->set_source_rgb(0.7, 0.1, 0.1);
    if(this->paths) {
        for(int i = 0 ; i < this->paths->size() ; i++) {
            svg::Path path = this->paths->at(i);

            svg::Transformation t = svg::Translation(bedX + offset_x * bed_scale, bedY - offset_y * bed_scale) * svg::Scale(bed_scale, bed_scale) * path.getTransformation();
            for(int j = 0 ; j < path.size() ; j++) {
                svg::PathElement *element = path.at(j);
                svg::CubicBezier *cb = dynamic_cast<svg::CubicBezier*>(element);
                svg::QuadraticBezier *qb = dynamic_cast<svg::QuadraticBezier*>(element);
                svg::Line *l = dynamic_cast<svg::Line*>(element);
                if(cb) {
                    svg::Point p1, p2, p3 ,p4;
                    p1 = cb->getP1() * t;
                    p2 = cb->getP2() * t;
                    p3 = cb->getP3() * t;
                    p4 = cb->getP4() * t;
                    ctx->move_to(p1.x, p1.y);
                    ctx->curve_to(p2.x, p2.y, p3.x, p3.y, p4.x, p4.y);
                } else if(qb) {
                    svg::Point p1, p2, p3;
                    p1 = qb->getP1() * t;
                    p2 = qb->getP2() * t;
                    p3 = qb->getP3() * t;
                    ctx->move_to(p1.x, p1.y);
                    ctx->curve_to(
                        p1.x + 2.0/3.0 * (p2.x - p1.x), p1.y + 2.0/3.0 * (p2.y - p1.y),
                        p3.x + 2.0/3.0 * (p2.x - p3.x), p3.y + 2.0/3.0 * (p2.y - p3.y),
                        p3.x, p3.y
                    );
                } else if(l) {
                    svg::Point p1, p2;
                    p1 = l->getP1() * t;
                    p2 = l->getP2() * t;
                    ctx->move_to(p1.x, p1.y);
                    ctx->line_to(p2.x, p2.y);
                }
            }
            ctx->stroke();
        }
        
    }

    //Border
    ctx->set_source_rgb(0.1, 0.1, 0.1);
    ctx->set_line_width(8.0);
    ctx->rectangle(0, 0, width, height);
    ctx->stroke();

    ctx->restore();

    return true;
}

Gtk::TreeModel::iterator Interface::addProperty(Glib::ustring property, double value) {
    const Gtk::TreeModel::iterator row = this->refPropertiesListStore->append();
    (*row)[this->propertiesModel.m_col_property] = property;
    (*row)[this->propertiesModel.m_col_value] = value;
    return row;
}

double Interface::getRowValue(Gtk::TreeModel::iterator row) {
    return row->get_value(this->propertiesModel.m_col_value);
}

void Interface::requestDraw(const Gtk::ListStore::Path&, const Gtk::ListStore::iterator&) {
    this->drawingArea->queue_draw();
}

void Interface::saveConfig() {
    fstream config;
    config.open("config.lwc", ios::out);
    if(config.good()) {
        config << this->getRowValue(this->rowOffsetX) << "\036";
        config << this->getRowValue(this->rowOffsetY) << "\036";
        config << this->getRowValue(this->rowBedWidth) << "\036";
        config << this->getRowValue(this->rowBedHeight) << "\036";
        config << this->getRowValue(this->rowTravelSpeed) << "\036";
        config << this->getRowValue(this->rowWorkingSpeed) << "\036";
        config << this->startGcodeTextView->get_buffer()->get_text() << "\036";
        config << this->endGcodeTextView->get_buffer()->get_text() << "\036";
        config << this->toolOnGcodeTextView->get_buffer()->get_text() << "\036";
        config << this->toolOffGcodeTextView->get_buffer()->get_text() << "\036";
    }
}

bool Interface::loadConfig() {
    ifstream config;
    config.open("config.lwc", ios::binary);
    unsigned int length = config.tellg();
    config.seekg(0, ios::end);
    length = static_cast<unsigned int>(config.tellg()) - length;
    config.seekg(0, ios::beg);
    char *cstr = new char[length];
    config.read(cstr, length);
    config.close();

    vector<string> strings;
    int j = 0;
    for(int i = 0 ; i < length ; i++) {
        if(j == strings.size()) {
            strings.push_back(string());
        }
        if(cstr[i] == '\036') {
            j++;
        } else {
            strings[j].push_back(cstr[i]);
        }
    }
    delete[] cstr;

    if(strings.size() == 10) {
        try {
            this->rowOffsetX = this->addProperty("Offset X (mm)", parseDouble(strings[0]));
            this->rowOffsetY = this->addProperty("Offset Y (mm)", parseDouble(strings[1]));
            this->rowBedWidth = this->addProperty("Bed width (mm)", parseDouble(strings[2]));
            this->rowBedHeight = this->addProperty("Bed height (mm)", parseDouble(strings[3]));
            this->rowTravelSpeed = this->addProperty("Travel speed (mm/s)", parseDouble(strings[4]));
            this->rowWorkingSpeed = this->addProperty("Working speed (mm/s)", parseDouble(strings[5]));
            this->startGcodeTextView->get_buffer()->set_text(strings[6]);
            this->endGcodeTextView->get_buffer()->set_text(strings[7]);
            this->toolOnGcodeTextView->get_buffer()->set_text(strings[8]);
            this->toolOffGcodeTextView->get_buffer()->set_text(strings[9]);
            return true;
        } catch(invalid_argument& e) {
            return false;
        }
    } else return false;
}
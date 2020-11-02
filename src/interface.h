#pragma once

#include <gtkmm.h>
#include <thread>
#include "svg.h"


class Interface {
private:
    Glib::RefPtr<Gtk::Application> app;
    Gtk::Window *window;

    Gtk::Button *loadSvgButton, *exportGcodeButton;
    Gtk::TextView *startGcodeTextView, *endGcodeTextView, *toolOnGcodeTextView, *toolOffGcodeTextView;
    Gtk::DrawingArea *drawingArea;
    Gtk::MenuItem *loadSvgMenuItem, *exportGcodeMenuItem, *exitMenuItem, *aboutMenuItem;

    std::vector<svg::Path> *paths;

    class PropertiesModel : public Gtk::TreeModel::ColumnRecord {
    public:
        PropertiesModel() {
            add(m_col_property);
            add(m_col_value);
        }
        Gtk::TreeModelColumn<Glib::ustring> m_col_property;
        Gtk::TreeModelColumn<double> m_col_value;
    };
    Gtk::TreeView *propertiesTreeView;
    Glib::RefPtr<Gtk::ListStore> refPropertiesListStore;
    PropertiesModel propertiesModel;

    Gtk::TreeModel::iterator addProperty(Glib::ustring property, double value);
    double getRowValue(Gtk::TreeModel::iterator);
    Gtk::TreeModel::iterator rowOffsetX, rowOffsetY, rowBedWidth, rowBedHeight, rowTravelSpeed, rowWorkingSpeed;

    Glib::RefPtr<Gdk::Pixbuf> icon;


public:
    static const std::string windowName;

    Interface();
    ~Interface();

    bool draw(const Cairo::RefPtr<Cairo::Context> &ccontext);
    void run();
    void loadSvgButtonClicked();
    void exportGcodeButtonClicked();
    void requestDraw(const Gtk::ListStore::Path&, const Gtk::ListStore::iterator&);
    void saveConfig();
    bool loadConfig();
};
#include "attackmovewindow.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QDebug>
#include <QSpinBox>

#include "mainwindow.hpp"

// Returns {min_hits, max_hits} for multi-hit moves, {0, 0} for non-multi-hit.
static std::pair<int,int> multiHitRange(Moves m) {
    switch(m) {
        case Moves::Bullet_Seed:
        case Moves::Icicle_Spear:
        case Moves::Pin_Missile:
        case Moves::Rock_Blast:
        case Moves::Tail_Slap:
        case Moves::Scale_Shot:
        case Moves::Water_Shuriken:
            return {2, 5};
        case Moves::Arm_Thrust:
            return {3, 5};
        case Moves::Population_Bomb:
            return {1, 10};
        case Moves::Double_Hit:
        case Moves::Double_Kick:
        case Moves::Dual_Chop:
        case Moves::Dual_Wingbeat:
        case Moves::Bonemerang:
        case Moves::Gear_Grind:
        case Moves::Tachyon_Cutter:
            return {2, 2};
        case Moves::Surging_Strikes:
        case Moves::Triple_Axel:
            return {3, 3};
        default:
            return {0, 0};
    }
}

//a lot of this class is basicly recycled from defensemovewindow.cpp, maybe they should inherit from a common source

AttackMoveWindow::AttackMoveWindow(QWidget* parent, Qt::WindowFlags f) : QDialog(parent, f) {
    edit_mode = false;

    QVBoxLayout* main_layout = new QVBoxLayout;
    setLayout(main_layout);

    createDefendingPokemonGroupbox();
    createModifierGroupbox();
    createMoveGroupbox();

    main_layout->addWidget(defending_pokemon_groupbox);
    main_layout->addWidget(move_groupbox);
    main_layout->addWidget(move_modifier_groupbox);
    main_layout->addWidget(atk_modifier_groupbox);

    bottom_button_box = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    main_layout->addWidget(bottom_button_box, Qt::AlignRight);

    //SIGNALS
    connect(bottom_button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bottom_button_box, SIGNAL(accepted()), this, SLOT(solveMove(void)));
    connect(bottom_button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void AttackMoveWindow::createDefendingPokemonGroupbox() {
    //the main horizontal layout for this window
    QHBoxLayout* def_layout = new QHBoxLayout;

    defending_pokemon_groupbox = new QGroupBox(tr("Defending Pokemon:"));
    defending_pokemon_groupbox->setLayout(def_layout);

    //---SPECIES & TYPES---//

    //---species---//
    //the main layout for this entire section
    QVBoxLayout* species_types_layout = new QVBoxLayout;
    species_types_layout->setSpacing(1);
    species_types_layout->setAlignment(Qt::AlignVCenter);

    //---sprite---
    QHBoxLayout* sprite_layout = new QHBoxLayout;

    QLabel* sprite = new QLabel;
    sprite->setObjectName("def_sprite");
    sprite->setAlignment(Qt::AlignCenter);
    sprite_layout->addWidget(sprite);

    species_types_layout->addLayout(sprite_layout);

    //some spacing
    species_types_layout->insertSpacing(1, 10);

    //---species---//
    //the layout just for the species
    QHBoxLayout* species_layout = new QHBoxLayout;
    species_types_layout->addLayout(species_layout);

    //the combobox for all the species
    QComboBox* species = new QComboBox;
    species->setObjectName("def_species_combobox");

    //loading it with all the species name
    MainWindow::populateSortedComboBox(species, ((MainWindow*)parentWidget())->getSpeciesNames());

    //some resizing
    int species_width = species->minimumSizeHint().width();
    species->setMaximumWidth(species_width);
    //adding it to the layout
    species_layout->addWidget(species);

    //---forms---//
    //the layout just for the forms
    QHBoxLayout* forms_layout = new QHBoxLayout;
    species_types_layout->addLayout(forms_layout);

    //the combobox for all the species
    QComboBox* forms = new QComboBox;
    forms->setObjectName("def_forms_combobox");

    //some resizing
    int forms_width = forms->minimumSizeHint().width();
    forms->setMaximumWidth(forms_width);
    //adding it to the layout
    forms_layout->addWidget(forms);

    //---types---//
    //the layout for the types
    QHBoxLayout* types_layout = new QHBoxLayout;

    //the comboboxes for the types
    QComboBox* types1 = new QComboBox;
    QComboBox* types2 = new QComboBox;
    types1->setObjectName("def_type1_combobox");
    types2->setObjectName("def_type2_combobox");

    //loading it with all the types names
    MainWindow::populateSortedComboBox(types1, ((MainWindow*)parentWidget())->getTypesNames());
    MainWindow::populateSortedComboBox(types2, ((MainWindow*)parentWidget())->getTypesNames());

    //resizing them
    int types_width = species->minimumSizeHint().width();
    types1->setMaximumWidth(types_width);
    types2->setMaximumWidth(types_width);

    //adding them to the layout
    types_layout->addWidget(types1);
    types_layout->addWidget(types2);
    species_types_layout->addLayout(types_layout);

    //adding everything to the window
    def_layout->addLayout(species_types_layout);

    //and then some spacing
    def_layout->insertSpacing(1, 35);

    //---MAIN FORM---//
    QVBoxLayout* main_form_layout = new QVBoxLayout;

    //---information section---///
    QFormLayout* form_layout = new QFormLayout;
    main_form_layout->addLayout(form_layout);

    //NATURE
    //the combobox for the nature
    QComboBox* natures = new QComboBox;
    natures->setObjectName("def_nature_combobox");

    //populating it
    MainWindow::populateSortedComboBox(natures, ((MainWindow*)parentWidget())->getNaturesNames());

    form_layout->addRow(tr("Nature:"), natures);

    //ABILITY
    //the combobox for the ability
    QComboBox* abilities = new QComboBox;
    abilities->setObjectName("def_abilities_combobox");

    //populating it
    MainWindow::populateSortedComboBox(abilities, ((MainWindow*)parentWidget())->getAbilitiesNames());

    //getting the abilities width (to be used later)
    int abilities_width = abilities->minimumSizeHint().width();

    form_layout->addRow(tr("Ability:"), abilities);

    //ITEM
    //the combobox for the item
    QComboBox* items = new QComboBox;
    items->setObjectName("def_items_combobox");

    //populating it
    MainWindow::populateSortedComboBox(items, ((MainWindow*)parentWidget())->getItemsNames());

    form_layout->addRow(tr("Item:"), items);

    //TERA TYPE
    QComboBox* tera_type = new QComboBox;
    tera_type->setObjectName("def_teratype_combobox");
    MainWindow::populateSortedComboBox(tera_type, ((MainWindow*)parentWidget())->getTypesNames());
    tera_type->setMaximumWidth(abilities_width);
    form_layout->addRow(tr("Tera Type:"), tera_type);

    //TERASTALLIZED
    QCheckBox* terastallized = new QCheckBox;
    terastallized->setObjectName("def_terastallized");
    form_layout->addRow(tr("Terastallized:"), terastallized);

    natures->setMaximumWidth(abilities_width);
    items->setMaximumWidth(abilities_width);
    abilities->setMaximumWidth(abilities_width);

    //some spacing
    main_form_layout->insertSpacing(1, 10);

    //---modifiers---//
    QHBoxLayout* modifiers_hp_layout = new QHBoxLayout;
    modifiers_hp_layout->setAlignment(Qt::AlignLeft);
    modifiers_hp_layout->setSpacing(10);

    //hp iv
    QLabel* hp_iv_label = new QLabel(tr("HP IV:"));
    hp_iv_label->setObjectName("hp_iv_label");
    modifiers_hp_layout->addWidget(hp_iv_label);

    QSpinBox* hp_iv = new QSpinBox;
    hp_iv->setObjectName("hp_iv_spinbox");
    hp_iv->setRange(0, 31);
    hp_iv->setValue(31);
    modifiers_hp_layout->addWidget(hp_iv);

    //hp ev
    QLabel* hp_ev_label = new QLabel(tr("HP EV:"));
    hp_ev_label->setObjectName("hp_ev_label");
    modifiers_hp_layout->addWidget(hp_ev_label);

    QSpinBox* hp_ev = new QSpinBox;
    hp_ev->setObjectName("hp_ev_spinbox");
    hp_ev->setRange(0, 32);
    modifiers_hp_layout->addWidget(hp_ev);

    //hp perc
    QLabel* hp_perc_label = new QLabel(tr("HP percentage:"));
    hp_perc_label->setObjectName("hp_perc_label");
    modifiers_hp_layout->addWidget(hp_perc_label);

    QSpinBox* hp_perc = new QSpinBox;
    hp_perc->setObjectName("hp_perc_spinbox");
    hp_perc->setRange(0, 100);
    hp_perc->setValue(100);
    hp_perc->setSuffix("%");
    modifiers_hp_layout->addWidget(hp_perc);

    main_form_layout->addLayout(modifiers_hp_layout);

    QHBoxLayout* modifiers_layout = new QHBoxLayout;
    modifiers_layout->setAlignment(Qt::AlignLeft);
    modifiers_layout->setSpacing(10);

    //iv
    QLabel* iv_label = new QLabel(tr("Def IV:"));
    iv_label->setObjectName("def_iv_label");
    modifiers_layout->addWidget(iv_label);

    QSpinBox* iv = new QSpinBox;
    iv->setObjectName("def_iv_spinbox");
    iv->setRange(0, 31);
    iv->setValue(31);
    modifiers_layout->addWidget(iv);

    //ev
    QLabel* ev_label = new QLabel(tr("Def EV:"));
    ev_label->setObjectName("def_ev_label");
    modifiers_layout->addWidget(ev_label);

    QSpinBox* ev = new QSpinBox;
    ev->setRange(0, 32);
    ev->setObjectName("def_ev_spinbox");
    modifiers_layout->addWidget(ev);

    //modifier
    QLabel* modifier_label = new QLabel(tr("Def Modifier:"));
    modifier_label->setObjectName("def_modifier_label");
    modifiers_layout->addWidget(modifier_label);

    QSpinBox* modifier = new QSpinBox;
    modifier->setObjectName("def_modifier_spinbox");
    modifier->setRange(-6, 6);
    modifiers_layout->addWidget(modifier);

    main_form_layout->addLayout(modifiers_layout);

    def_layout->addLayout(main_form_layout);

    //setting signals
    connect(species, SIGNAL(currentIndexChanged(int)), this, SLOT(setSpecies(int)));
    connect(forms, SIGNAL(currentIndexChanged(int)), this, SLOT(setForm(int)));

    //setting index 0
    species->setCurrentIndex(1); //setting it to 1 because the signal is currentindexCHANGED, i am so stupid lol
    species->setCurrentIndex(0);
}

void AttackMoveWindow::createMoveGroupbox() {
    //---moves---//
    QVBoxLayout* move_layout = new QVBoxLayout;
    move_layout->setAlignment(Qt::AlignLeft);

    move_groupbox = new QGroupBox(tr("Move:"));
    move_groupbox->setLayout(move_layout);

    //label
    QHBoxLayout* move_name_layout = new QHBoxLayout;
    move_name_layout->setAlignment(Qt::AlignLeft);
    QLabel* move_name_label = new QLabel(tr("Move:"));
    move_name_layout->addWidget(move_name_label);

    //name
    //the combobox for the move
    QComboBox* moves = new QComboBox;
    moves->setObjectName("moves_combobox");

    //populating it
    MainWindow::populateSortedComboBox(moves, ((MainWindow*)parentWidget())->getMovesNames());

    move_name_layout->addWidget(moves);

    move_layout->addLayout(move_name_layout);

    //target
    QHBoxLayout* move_info_layout = new QHBoxLayout;

    QComboBox* target = new QComboBox;
    target->setObjectName("target_combobox");

    //populating it
    target->addItem(tr("Single Target"));
    target->addItem(tr("Double Target"));

    move_info_layout->addWidget(target);

    //type
    QComboBox* move_types = new QComboBox;
    move_types->setObjectName("movetypes_combobox");

    //populating it
    MainWindow::populateSortedComboBox(move_types, ((MainWindow*)parentWidget())->getTypesNames());

    move_info_layout->addWidget(move_types);

    move_layout->addLayout(move_info_layout);
    //main_form_layout->addLayout(move_layout);

    //category
    QComboBox* move_category = new QComboBox;
    move_category->setObjectName("movecategories_combobox");

    //populating it
    move_category->addItem("Physical");
    move_category->addItem("Special");

    move_info_layout->addWidget(move_category);

    //bp
    QSpinBox* move_bp = new QSpinBox;
    move_bp->setObjectName("movebp_spinbox");
    move_bp->setRange(1, 999);

    move_info_layout->addWidget(move_bp);

    //crit
    QLabel* crit_label = new QLabel(tr("Crit"));
    move_info_layout->addWidget(crit_label);

    QCheckBox* crit = new QCheckBox;
    crit->setObjectName("crit");
    move_info_layout->addWidget(crit);

    //z
    QLabel* z_label = new QLabel(tr("Z"));
    move_info_layout->addWidget(z_label);

    QCheckBox* z = new QCheckBox;
    z->setObjectName("z");
    move_info_layout->addWidget(z);

    //multi-hit (shown only for multi-hit moves)
    QLabel* multihit_label = new QLabel(tr("Hits:"));
    multihit_label->setObjectName("move_multihit_label");
    multihit_label->setVisible(false);
    move_info_layout->addWidget(multihit_label);

    QSpinBox* multihit_spinbox = new QSpinBox;
    multihit_spinbox->setObjectName("move_multihit_spinbox");
    multihit_spinbox->setRange(1, 10);
    multihit_spinbox->setValue(1);
    multihit_spinbox->setVisible(false);
    move_info_layout->addWidget(multihit_spinbox);

    //setting signals
    connect(moves, SIGNAL(currentIndexChanged(int)), this, SLOT(setMove(int)));
    connect(move_category, SIGNAL(currentIndexChanged(int)), this, SLOT(setMoveCategory(int)));

    //setting index 0
    moves->setCurrentIndex(1); //setting it to 1 because the signal is currentindexCHANGED, i am so stupid lol
    moves->setCurrentIndex(0);
}

void AttackMoveWindow::createModifierGroupbox() {
    move_modifier_groupbox = new QGroupBox(tr("Modifiers:"));

    atk_modifier_groupbox = new QGroupBox(tr("Attacking Pokemon Modifiers:"));

    QHBoxLayout* attacking_layout = new QHBoxLayout;
    atk_modifier_groupbox->setLayout(attacking_layout);

    QLabel* atk_modifier_label = new QLabel;
    atk_modifier_label->setObjectName("attacking_atk_modifier_label");
    atk_modifier_label->setText(tr("Attack modifier"));
    attacking_layout->addWidget(atk_modifier_label);

    QSpinBox* atk_modifier_spinbox = new QSpinBox;
    atk_modifier_spinbox->setObjectName("attacking_atk_modifier");
    atk_modifier_spinbox->setRange(-6, 6);
    attacking_layout->addWidget(atk_modifier_spinbox, Qt::AlignLeft);

    QLabel* hits_modifier_label = new QLabel;
    hits_modifier_label->setText(tr("Number of hits:"));
    attacking_layout->addWidget(hits_modifier_label);

    QSpinBox* hits_modifier_spinbox = new QSpinBox;
    hits_modifier_spinbox->setObjectName("attacking_hits_modifier");
    hits_modifier_spinbox->setRange(1, 4);
    hits_modifier_spinbox->setSuffix("HKO");
    attacking_layout->addWidget(hits_modifier_spinbox, Qt::AlignLeft);

    QLabel* atk_tera_label = new QLabel(tr("Tera Type:"));
    attacking_layout->addWidget(atk_tera_label);

    QComboBox* atk_tera_combobox = new QComboBox;
    atk_tera_combobox->setObjectName("attacking_tera_type");
    MainWindow::populateSortedComboBox(atk_tera_combobox, ((MainWindow*)parentWidget())->getTypesNames());
    attacking_layout->addWidget(atk_tera_combobox, Qt::AlignLeft);

    QLabel* atk_tera_bool_label = new QLabel(tr("Terastallized:"));
    attacking_layout->addWidget(atk_tera_bool_label);

    QCheckBox* atk_terastallized = new QCheckBox;
    atk_terastallized->setObjectName("attacking_terastallized");
    attacking_layout->addWidget(atk_terastallized, Qt::AlignLeft);

    move_modifier_groupbox = new QGroupBox("Modifiers:");

    QHBoxLayout* modifier_layout = new QHBoxLayout;
    modifier_layout->setAlignment(Qt::AlignLeft);

    move_modifier_groupbox->setLayout(modifier_layout);

    //WEATHER
    QHBoxLayout* weather_layout = new QHBoxLayout;
    weather_layout->setAlignment(Qt::AlignLeft);

    QLabel* weather_label = new QLabel(tr("Weather:"));
    weather_layout->addWidget(weather_label);

    QComboBox* weather_combobox = new QComboBox;
    weather_combobox->setObjectName("weather_combobox");

    weather_combobox->addItem(tr("None"));
    weather_combobox->addItem(tr("Sun"));
    weather_combobox->addItem(tr("Rain"));
    weather_combobox->addItem(tr("Harsh Sunshine"));
    weather_combobox->addItem(tr("Heavy Rain"));
    weather_combobox->addItem(tr("Strong Winds"));

    weather_layout->addWidget(weather_combobox);

    modifier_layout->addLayout(weather_layout);

    //TERRAIN
    QHBoxLayout* terrain_layout = new QHBoxLayout;
    terrain_layout->setAlignment(Qt::AlignLeft);

    QLabel* terrain_label = new QLabel(tr("Terrain:"));
    terrain_layout->addWidget(terrain_label);

    QComboBox* terrain_combobox = new QComboBox;
    terrain_combobox->setObjectName("terrain_combobox");

    terrain_combobox->addItem(tr("None"));
    terrain_combobox->addItem(tr("Grassy"));
    terrain_combobox->addItem(tr("Electric"));
    terrain_combobox->addItem(tr("Psychic"));
    terrain_combobox->addItem(tr("Misty"));

    terrain_layout->addWidget(terrain_combobox);

    modifier_layout->addLayout(terrain_layout);

    //RUIN ABILITIES + HELPING HAND
    QHBoxLayout* ruin_layout = new QHBoxLayout;
    ruin_layout->setAlignment(Qt::AlignLeft);

    QLabel* ruin_label = new QLabel(tr("Field:"));
    ruin_layout->addWidget(ruin_label);

    QLabel* tablets_label = new QLabel(tr("Tablets (−25% Atk)"));
    ruin_layout->addWidget(tablets_label);
    QCheckBox* tablets_cb = new QCheckBox;
    tablets_cb->setObjectName("tablets_of_ruin_checkbox");
    ruin_layout->addWidget(tablets_cb);

    QLabel* vessel_label = new QLabel(tr("Vessel (−25% SpAtk)"));
    ruin_layout->addWidget(vessel_label);
    QCheckBox* vessel_cb = new QCheckBox;
    vessel_cb->setObjectName("vessel_of_ruin_checkbox");
    ruin_layout->addWidget(vessel_cb);

    QLabel* sword_label = new QLabel(tr("Sword (−25% Def)"));
    ruin_layout->addWidget(sword_label);
    QCheckBox* sword_cb = new QCheckBox;
    sword_cb->setObjectName("sword_of_ruin_checkbox");
    ruin_layout->addWidget(sword_cb);

    QLabel* beads_label = new QLabel(tr("Beads (−25% SpDef)"));
    ruin_layout->addWidget(beads_label);
    QCheckBox* beads_cb = new QCheckBox;
    beads_cb->setObjectName("beads_of_ruin_checkbox");
    ruin_layout->addWidget(beads_cb);

    QLabel* hh_label = new QLabel(tr("Helping Hand (×1.5)"));
    ruin_layout->addWidget(hh_label);
    QCheckBox* hh_cb = new QCheckBox;
    hh_cb->setObjectName("helping_hand_checkbox");
    ruin_layout->addWidget(hh_cb);

    QLabel* fg_label = new QLabel(tr("Friend Guard (×0.75)"));
    ruin_layout->addWidget(fg_label);
    QCheckBox* fg_cb = new QCheckBox;
    fg_cb->setObjectName("friend_guard_checkbox");
    ruin_layout->addWidget(fg_cb);

    modifier_layout->addLayout(ruin_layout);
}

void AttackMoveWindow::setMove(int index) {
    int origIdx = move_groupbox->findChild<QComboBox*>("moves_combobox")->currentData(Qt::UserRole).toInt();
    Move selected_move((Moves)origIdx);

    MainWindow::setComboByOriginalIdx(move_groupbox->findChild<QComboBox*>("movetypes_combobox"), selected_move.getMoveType());

    QComboBox* target = move_groupbox->findChild<QComboBox*>("target_combobox");
    if( selected_move.isSpread() ) { target->setCurrentIndex(Move::Target::DOUBLE); target->setVisible(true); }
    else { target->setCurrentIndex(Move::Target::SINGLE); target->setVisible(false); }

    move_groupbox->findChild<QComboBox*>("movecategories_combobox")->setCurrentIndex(selected_move.getMoveCategory());
    move_groupbox->findChild<QSpinBox*>("movebp_spinbox")->setValue(selected_move.getBasePower());

    if( selected_move.isSignatureZ() ) move_groupbox->findChild<QCheckBox*>("z")->setEnabled(false);
    else move_groupbox->findChild<QCheckBox*>("z")->setEnabled(true);

    // Surging Strikes and Wicked Blow always land critical hits
    if( (Moves)index == Moves::Surging_Strikes || (Moves)index == Moves::Wicked_Blow )
        move_groupbox->findChild<QCheckBox*>("crit")->setChecked(true);

    if( selected_move.getMoveCategory() == Move::Category::PHYSICAL ) {
        defending_pokemon_groupbox->findChild<QLabel*>("def_iv_label")->setText(tr("Def IV"));
        defending_pokemon_groupbox->findChild<QLabel*>("def_ev_label")->setText(tr("Def EV"));
        defending_pokemon_groupbox->findChild<QLabel*>("def_modifier_label")->setText(tr("Def Modifier"));
        atk_modifier_groupbox->findChild<QLabel*>("attacking_atk_modifier_label")->setText(tr("Attack Modifier"));
    }

    else {
        defending_pokemon_groupbox->findChild<QLabel*>("def_iv_label")->setText(tr("Sp. Def IV"));
        defending_pokemon_groupbox->findChild<QLabel*>("def_ev_label")->setText(tr("Sp. Def EV"));
        defending_pokemon_groupbox->findChild<QLabel*>("def_modifier_label")->setText(tr("Sp. Def Modifier"));
        atk_modifier_groupbox->findChild<QLabel*>("attacking_atk_modifier_label")->setText(tr("Sp. Attack Modifier"));
    }

    // Multi-hit support
    {
        auto [min_h, max_h] = multiHitRange((Moves)origIdx);
        QLabel* mh_label = move_groupbox->findChild<QLabel*>("move_multihit_label");
        QSpinBox* mh_spinbox = move_groupbox->findChild<QSpinBox*>("move_multihit_spinbox");
        if(max_h > 0) {
            mh_spinbox->setRange(min_h, max_h);
            mh_spinbox->setValue(min_h);
            mh_spinbox->setEnabled(min_h != max_h);
            mh_label->setVisible(true);
            mh_spinbox->setVisible(true);
        } else {
            mh_label->setVisible(false);
            mh_spinbox->setVisible(false);
        }
    }
}
void AttackMoveWindow::setSpecies(int index) {
    int orig = defending_pokemon_groupbox->findChild<QComboBox*>("def_species_combobox")->currentData(Qt::UserRole).toInt();
    Pokemon selected_pokemon(orig + 1);

    //setting correct sprite
    QPixmap sprite_pixmap;
    QString sprite_path = ":/db/sprites/" + QString::number(selected_pokemon.getPokedexNumber()) + ".png";
    sprite_pixmap.load(sprite_path);
    const int SPRITE_SCALE_FACTOR = 2;
    sprite_pixmap = sprite_pixmap.scaled(sprite_pixmap.width() * SPRITE_SCALE_FACTOR, sprite_pixmap.height() * SPRITE_SCALE_FACTOR);

    QLabel* sprite = defending_pokemon_groupbox->findChild<QLabel*>("def_sprite");
    sprite->setPixmap(sprite_pixmap);

    //setting correct types
    QComboBox* type1 = defending_pokemon_groupbox->findChild<QComboBox*>("def_type1_combobox");
    MainWindow::setComboByOriginalIdx(type1, selected_pokemon.getTypes()[0][0]);

    QComboBox* type2 = defending_pokemon_groupbox->findChild<QComboBox*>("def_type2_combobox");
    MainWindow::setComboByOriginalIdx(type2, selected_pokemon.getTypes()[0][1]);

    if( selected_pokemon.getTypes()[0][0] == selected_pokemon.getTypes()[0][1] ) { type2->setVisible(false); type2->setVisible(false); }
    else { type2->setVisible(true); type2->setVisible(true); }

    //setting correct form
    {
        QComboBox* form = defending_pokemon_groupbox->findChild<QComboBox*>("def_forms_combobox");
        MainWindow::populateFormCombo(form, orig + 1, selected_pokemon.getFormesNumber());
        form->setCurrentIndex(0);
        form->setVisible(form->count() > 1);
    }

    //setting correct ability
    QComboBox* ability = defending_pokemon_groupbox->findChild<QComboBox*>("def_abilities_combobox");
    MainWindow::setComboByOriginalIdx(ability, selected_pokemon.getPossibleAbilities()[0][0]);
}

void AttackMoveWindow::setForm(int index) {
    int orig = defending_pokemon_groupbox->findChild<QComboBox*>("def_species_combobox")->currentData(Qt::UserRole).toInt();
    Pokemon selected_pokemon(orig + 1);

    int form_idx = defending_pokemon_groupbox->findChild<QComboBox*>("def_forms_combobox")->currentData(Qt::UserRole).toInt();

    //setting correct sprite
    QPixmap sprite_pixmap;
    QString sprite_path;
    bool load_result;

    if( form_idx == 0 ) sprite_path = ":/db/sprites/" + QString::number(selected_pokemon.getPokedexNumber()) + ".png";
    else sprite_path = ":/db/sprites/" + QString::number(selected_pokemon.getPokedexNumber()) + "-" + QString::number(form_idx) + ".png";

    sprite_pixmap.load(sprite_path);
    const int SPRITE_SCALE_FACTOR = 2;
    sprite_pixmap = sprite_pixmap.scaled(sprite_pixmap.width() * SPRITE_SCALE_FACTOR, sprite_pixmap.height() * SPRITE_SCALE_FACTOR);

    QLabel* sprite = defending_pokemon_groupbox->findChild<QLabel*>("def_sprite");
    sprite->setPixmap(sprite_pixmap);

    //setting correct types
    QComboBox* type1 = defending_pokemon_groupbox->findChild<QComboBox*>("def_type1_combobox");
    MainWindow::setComboByOriginalIdx(type1, selected_pokemon.getTypes()[form_idx][0]);

    QComboBox* type2 = defending_pokemon_groupbox->findChild<QComboBox*>("def_type2_combobox");
    MainWindow::setComboByOriginalIdx(type2, selected_pokemon.getTypes()[form_idx][1]);

    if( selected_pokemon.getTypes()[form_idx][0] == selected_pokemon.getTypes()[form_idx][1] ) { type2->setVisible(false); type2->setVisible(false); }
    else { type2->setVisible(true); type2->setVisible(true); }

    //setting correct ability
    QComboBox* ability = defending_pokemon_groupbox->findChild<QComboBox*>("def_abilities_combobox");
    MainWindow::setComboByOriginalIdx(ability, selected_pokemon.getPossibleAbilities()[form_idx][0]);
}

void AttackMoveWindow::solveMove(void) {
    Pokemon attacking1(defending_pokemon_groupbox->findChild<QComboBox*>("def_species_combobox")->currentData(Qt::UserRole).toInt()+1);
    attacking1.setForm(defending_pokemon_groupbox->findChild<QComboBox*>("def_forms_combobox")->currentData(Qt::UserRole).toInt());

    attacking1.setType(0, (Type)defending_pokemon_groupbox->findChild<QComboBox*>("def_type1_combobox")->currentData(Qt::UserRole).toInt());
    attacking1.setType(1, (Type)defending_pokemon_groupbox->findChild<QComboBox*>("def_type2_combobox")->currentData(Qt::UserRole).toInt());
    attacking1.setNature((Stats::Nature)defending_pokemon_groupbox->findChild<QComboBox*>("def_nature_combobox")->currentData(Qt::UserRole).toInt());
    attacking1.setAbility((Ability)defending_pokemon_groupbox->findChild<QComboBox*>("def_abilities_combobox")->currentData(Qt::UserRole).toInt());
    attacking1.setItem(Item(defending_pokemon_groupbox->findChild<QComboBox*>("def_items_combobox")->currentData(Qt::UserRole).toInt()));
    attacking1.setTeraType((Type)defending_pokemon_groupbox->findChild<QComboBox*>("def_teratype_combobox")->currentData(Qt::UserRole).toInt());
    attacking1.setTerastallized(defending_pokemon_groupbox->findChild<QCheckBox*>("def_terastallized")->isChecked());

    Move attacking1_move((Moves)move_groupbox->findChild<QComboBox*>("moves_combobox")->currentData(Qt::UserRole).toInt());
    if( move_groupbox->findChild<QComboBox*>("target_combobox")->currentIndex() == Move::Target::SINGLE ) attacking1_move.setTarget(Move::Target::SINGLE);
    else attacking1_move.setTarget(Move::Target::DOUBLE);
    attacking1_move.setMoveType((Type)move_groupbox->findChild<QComboBox*>("movetypes_combobox")->currentData(Qt::UserRole).toInt());
    attacking1_move.setMoveCategory((Move::Category)move_groupbox->findChild<QComboBox*>("movecategories_combobox")->currentIndex());
    attacking1_move.setBasePower(move_groupbox->findChild<QSpinBox*>("movebp_spinbox")->value());

    if( move_groupbox->findChild<QCheckBox*>("crit")->checkState() == Qt::Checked ) attacking1_move.setCrit(true);
    else if(move_groupbox->findChild<QCheckBox*>("crit")->checkState() == Qt::Unchecked ) attacking1_move.setCrit(false);

    if( move_groupbox->findChild<QCheckBox*>("z")->checkState() == Qt::Checked ) attacking1_move.setZ(true);
    else if( move_groupbox->findChild<QCheckBox*>("z")->checkState() == Qt::Unchecked ) attacking1_move.setZ(false);

    attacking1_move.setWeather((Move::Weather)move_modifier_groupbox->findChild<QComboBox*>("weather_combobox")->currentIndex());
    attacking1_move.setTerrain((Move::Terrain)move_modifier_groupbox->findChild<QComboBox*>("terrain_combobox")->currentIndex());

    //now setting pokemon 1 iv/ev/modifier
    Stats::Stat stat;
    if( attacking1_move.getMoveCategory() == Move::Category::PHYSICAL ) stat = Stats::DEF;
    else stat = Stats::SPDEF;

    attacking1.setIV(stat, defending_pokemon_groupbox->findChild<QSpinBox*>("def_iv_spinbox")->value());
    attacking1.setEV(stat, defending_pokemon_groupbox->findChild<QSpinBox*>("def_ev_spinbox")->value());
    attacking1.setModifier(stat, defending_pokemon_groupbox->findChild<QSpinBox*>("def_modifier_spinbox")->value());

    attacking1.setIV(Stats::HP, defending_pokemon_groupbox->findChild<QSpinBox*>("hp_iv_spinbox")->value());
    attacking1.setEV(Stats::HP, defending_pokemon_groupbox->findChild<QSpinBox*>("hp_ev_spinbox")->value());
    attacking1.setCurrentHPPercentage(defending_pokemon_groupbox->findChild<QSpinBox*>("hp_perc_spinbox")->value());

    // Apply multi-hit count if applicable
    {
        auto [min_h, max_h] = multiHitRange(attacking1_move.getMoveIndex());
        if(max_h > 0) {
            QSpinBox* mh_sb = move_groupbox->findChild<QSpinBox*>("move_multihit_spinbox");
            int count = (min_h != max_h && mh_sb) ? mh_sb->value() : max_h;
            attacking1_move.setMultiHitCount(count);
        }
    }

    Turn turn;
    turn.addMove(Pokemon(1), attacking1_move); //adding a random pokemon in the turn since when using the turn class in an offensive manner Pokemon is ignored
    turn.setHits(atk_modifier_groupbox->findChild<QSpinBox*>("attacking_hits_modifier")->value());

    int atk_mod;
    int spatk_mod;
    if( attacking1_move.getMoveCategory() == Move::Category::PHYSICAL ) { spatk_mod = 0; /*useless*/ atk_mod = atk_modifier_groupbox->findChild<QSpinBox*>("attacking_atk_modifier")->value(); }
    else { atk_mod = 0; spatk_mod = atk_modifier_groupbox->findChild<QSpinBox*>("attacking_atk_modifier")->value(); }

    Type atk_tera = (Type)atk_modifier_groupbox->findChild<QComboBox*>("attacking_tera_type")->currentData(Qt::UserRole).toInt();
    bool atk_terastallized = atk_modifier_groupbox->findChild<QCheckBox*>("attacking_terastallized")->isChecked();

    bool tablets_ruin = move_modifier_groupbox->findChild<QCheckBox*>("tablets_of_ruin_checkbox")->isChecked();
    bool vessel_ruin  = move_modifier_groupbox->findChild<QCheckBox*>("vessel_of_ruin_checkbox")->isChecked();
    bool sword_ruin   = move_modifier_groupbox->findChild<QCheckBox*>("sword_of_ruin_checkbox")->isChecked();
    bool beads_ruin   = move_modifier_groupbox->findChild<QCheckBox*>("beads_of_ruin_checkbox")->isChecked();
    bool helping_hand = move_modifier_groupbox->findChild<QCheckBox*>("helping_hand_checkbox")->isChecked();
    bool friend_guard = move_modifier_groupbox->findChild<QCheckBox*>("friend_guard_checkbox")->isChecked();

    ((MainWindow*)parentWidget())->addAttackTurn(turn, attacking1, std::make_tuple(atk_mod, spatk_mod, atk_tera, atk_terastallized, tablets_ruin, vessel_ruin, sword_ruin, beads_ruin, helping_hand, friend_guard));
}

void AttackMoveWindow::setAsBlank() {
    defending_pokemon_groupbox->findChild<QComboBox*>("def_species_combobox")->setCurrentIndex(0);
    defending_pokemon_groupbox->findChild<QComboBox*>("def_forms_combobox")->setCurrentIndex(0);
    MainWindow::setComboByOriginalIdx(defending_pokemon_groupbox->findChild<QComboBox*>("def_nature_combobox"), 0); // Hardy
    MainWindow::setComboByOriginalIdx(defending_pokemon_groupbox->findChild<QComboBox*>("def_items_combobox"), 0); // None
    defending_pokemon_groupbox->findChild<QComboBox*>("def_teratype_combobox")->setCurrentIndex(0);
    defending_pokemon_groupbox->findChild<QCheckBox*>("def_terastallized")->setChecked(false);

    defending_pokemon_groupbox->findChild<QSpinBox*>("def_iv_spinbox")->setValue(31);
    defending_pokemon_groupbox->findChild<QSpinBox*>("def_ev_spinbox")->setValue(0);
    defending_pokemon_groupbox->findChild<QSpinBox*>("def_modifier_spinbox")->setValue(0);

    defending_pokemon_groupbox->findChild<QSpinBox*>("hp_iv_spinbox")->setValue(31);
    defending_pokemon_groupbox->findChild<QSpinBox*>("hp_ev_spinbox")->setValue(0);
    defending_pokemon_groupbox->findChild<QSpinBox*>("hp_perc_spinbox")->setValue(100);

    move_groupbox->findChild<QComboBox*>("moves_combobox")->setCurrentIndex(0);
    move_groupbox->findChild<QCheckBox*>("crit")->setChecked(false);
    move_groupbox->findChild<QCheckBox*>("z")->setChecked(false);
    move_groupbox->findChild<QLabel*>("move_multihit_label")->setVisible(false);
    move_groupbox->findChild<QSpinBox*>("move_multihit_spinbox")->setVisible(false);
    move_groupbox->findChild<QSpinBox*>("move_multihit_spinbox")->setValue(1);

    move_modifier_groupbox->findChild<QComboBox*>("weather_combobox")->setCurrentIndex(0);
    move_modifier_groupbox->findChild<QComboBox*>("terrain_combobox")->setCurrentIndex(0);

    atk_modifier_groupbox->findChild<QSpinBox*>("attacking_atk_modifier")->setValue(0);
    atk_modifier_groupbox->findChild<QSpinBox*>("attacking_hits_modifier")->setValue(1);
    atk_modifier_groupbox->findChild<QComboBox*>("attacking_tera_type")->setCurrentIndex(0);
    atk_modifier_groupbox->findChild<QCheckBox*>("attacking_terastallized")->setChecked(false);
    move_modifier_groupbox->findChild<QCheckBox*>("tablets_of_ruin_checkbox")->setChecked(false);
    move_modifier_groupbox->findChild<QCheckBox*>("vessel_of_ruin_checkbox")->setChecked(false);
    move_modifier_groupbox->findChild<QCheckBox*>("sword_of_ruin_checkbox")->setChecked(false);
    move_modifier_groupbox->findChild<QCheckBox*>("beads_of_ruin_checkbox")->setChecked(false);
    move_modifier_groupbox->findChild<QCheckBox*>("helping_hand_checkbox")->setChecked(false);
    move_modifier_groupbox->findChild<QCheckBox*>("friend_guard_checkbox")->setChecked(false);
}

void AttackMoveWindow::setAsTurn(const Turn& theTurn, const Pokemon& theDefendingPokemon, const attack_modifier& theAttackModifier) {
    MainWindow::setComboByOriginalIdx(defending_pokemon_groupbox->findChild<QComboBox*>("def_species_combobox"), theDefendingPokemon.getPokedexNumber()-1);
    MainWindow::setComboByOriginalIdx(defending_pokemon_groupbox->findChild<QComboBox*>("def_type1_combobox"), theDefendingPokemon.getTypes()[theTurn.getMoves()[0].first.getForm()][0]);
    MainWindow::setComboByOriginalIdx(defending_pokemon_groupbox->findChild<QComboBox*>("def_type2_combobox"), theDefendingPokemon.getTypes()[theTurn.getMoves()[0].first.getForm()][1]);
    MainWindow::setFormComboByFormIdx(defending_pokemon_groupbox->findChild<QComboBox*>("def_forms_combobox"), theDefendingPokemon.getForm());
    MainWindow::setComboByOriginalIdx(defending_pokemon_groupbox->findChild<QComboBox*>("def_nature_combobox"), theDefendingPokemon.getNature());
    MainWindow::setComboByOriginalIdx(defending_pokemon_groupbox->findChild<QComboBox*>("def_items_combobox"), theDefendingPokemon.getItem().getIndex());
    MainWindow::setComboByOriginalIdx(defending_pokemon_groupbox->findChild<QComboBox*>("def_abilities_combobox"), theDefendingPokemon.getAbility());
    MainWindow::setComboByOriginalIdx(defending_pokemon_groupbox->findChild<QComboBox*>("def_teratype_combobox"), theDefendingPokemon.getTeraType());
    defending_pokemon_groupbox->findChild<QCheckBox*>("def_terastallized")->setChecked(theDefendingPokemon.isTerastallized());

    Stats::Stat stat;
    if( theTurn.getMoves()[0].second.getMoveCategory() == Move::Category::SPECIAL ) stat = Stats::SPDEF;
    else stat = Stats::DEF;

    defending_pokemon_groupbox->findChild<QSpinBox*>("def_iv_spinbox")->setValue(theDefendingPokemon.getIV(stat));
    defending_pokemon_groupbox->findChild<QSpinBox*>("def_ev_spinbox")->setValue(theDefendingPokemon.getEV(stat));
    defending_pokemon_groupbox->findChild<QSpinBox*>("def_modifier_spinbox")->setValue(theDefendingPokemon.getModifier(stat));
    defending_pokemon_groupbox->findChild<QSpinBox*>("hp_iv_spinbox")->setValue(theDefendingPokemon.getIV(Stats::HP));
    defending_pokemon_groupbox->findChild<QSpinBox*>("hp_ev_spinbox")->setValue(theDefendingPokemon.getEV(Stats::HP));
    defending_pokemon_groupbox->findChild<QSpinBox*>("hp_perc_spinbox")->setValue(theDefendingPokemon.getCurrentHPPercentage());

    MainWindow::setComboByOriginalIdx(move_groupbox->findChild<QComboBox*>("moves_combobox"), theTurn.getMoves()[0].second.getMoveIndex());
    // Restore multi-hit count (setMove() triggered by setComboByOriginalIdx set the spinbox range/visibility; now restore saved value)
    {
        QSpinBox* mh_spinbox = move_groupbox->findChild<QSpinBox*>("move_multihit_spinbox");
        if(mh_spinbox && mh_spinbox->isVisible())
            mh_spinbox->setValue((int)theTurn.getMoves()[0].second.getMultiHitCount());
    }
    move_groupbox->findChild<QComboBox*>("target_combobox")->setCurrentIndex(theTurn.getMoves()[0].second.getTarget());
    MainWindow::setComboByOriginalIdx(move_groupbox->findChild<QComboBox*>("movetypes_combobox"), theTurn.getMoves()[0].second.getMoveType());
    move_groupbox->findChild<QComboBox*>("movecategories_combobox")->setCurrentIndex(theTurn.getMoves()[0].second.getMoveCategory());
    move_groupbox->findChild<QSpinBox*>("movebp_spinbox")->setValue(theTurn.getMoves()[0].second.getBasePower());

    move_groupbox->findChild<QCheckBox*>("crit")->setChecked(theTurn.getMoves()[0].second.isCrit());
    move_groupbox->findChild<QCheckBox*>("z")->setChecked(theTurn.getMoves()[0].second.isZ());

    move_modifier_groupbox->findChild<QComboBox*>("weather_combobox")->setCurrentIndex(theTurn.getMoves()[0].second.getWeather());
    move_modifier_groupbox->findChild<QComboBox*>("terrain_combobox")->setCurrentIndex(theTurn.getMoves()[0].second.getTerrain());


    if( theTurn.getMoves()[0].second.getMoveCategory() == Move::Category::SPECIAL ) atk_modifier_groupbox->findChild<QSpinBox*>("attacking_atk_modifier")->setValue(std::get<1>(theAttackModifier));
    else atk_modifier_groupbox->findChild<QSpinBox*>("attacking_atk_modifier")->setValue(std::get<0>(theAttackModifier));

    atk_modifier_groupbox->findChild<QSpinBox*>("attacking_hits_modifier")->setValue(theTurn.getHits());
    MainWindow::setComboByOriginalIdx(atk_modifier_groupbox->findChild<QComboBox*>("attacking_tera_type"), std::get<2>(theAttackModifier));
    atk_modifier_groupbox->findChild<QCheckBox*>("attacking_terastallized")->setChecked(std::get<3>(theAttackModifier));
    move_modifier_groupbox->findChild<QCheckBox*>("tablets_of_ruin_checkbox")->setChecked(std::get<4>(theAttackModifier));
    move_modifier_groupbox->findChild<QCheckBox*>("vessel_of_ruin_checkbox")->setChecked(std::get<5>(theAttackModifier));
    move_modifier_groupbox->findChild<QCheckBox*>("sword_of_ruin_checkbox")->setChecked(std::get<6>(theAttackModifier));
    move_modifier_groupbox->findChild<QCheckBox*>("beads_of_ruin_checkbox")->setChecked(std::get<7>(theAttackModifier));
    move_modifier_groupbox->findChild<QCheckBox*>("helping_hand_checkbox")->setChecked(std::get<8>(theAttackModifier));
    move_modifier_groupbox->findChild<QCheckBox*>("friend_guard_checkbox")->setChecked(std::get<9>(theAttackModifier));
}

void AttackMoveWindow::setMoveCategory(int index) {
    if( index == Move::Category::PHYSICAL ) {
        defending_pokemon_groupbox->findChild<QLabel*>("def_iv_label")->setText(tr("Def IV"));
        defending_pokemon_groupbox->findChild<QLabel*>("def_ev_label")->setText(tr("Def EV"));
        defending_pokemon_groupbox->findChild<QLabel*>("def_modifier_label")->setText(tr("Def Modifier"));
        atk_modifier_groupbox->findChild<QLabel*>("attacking_atk_modifier_label")->setText(tr("Attack Modifier"));
    }

    else {
        defending_pokemon_groupbox->findChild<QLabel*>("def_iv_label")->setText(tr("Sp. Def IV"));
        defending_pokemon_groupbox->findChild<QLabel*>("def_ev_label")->setText(tr("Sp. Def EV"));
        defending_pokemon_groupbox->findChild<QLabel*>("def_modifier_label")->setText(tr("Sp. Def Modifier"));
        atk_modifier_groupbox->findChild<QLabel*>("attacking_atk_modifier_label")->setText(tr("Sp. Attack Modifier"));
    }
}

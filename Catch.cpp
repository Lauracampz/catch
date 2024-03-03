#include "Catch.h"
#include "ui_Catch.h"
#include "Player.h"

#include <QDebug>
#include <QMessageBox>
#include <QActionGroup>
#include <QSignalMapper>

Catch::Catch(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::Catch),
      m_player(Player::player(Player::Red)) {

    ui->setupUi(this);

    QObject::connect(ui->actionNew, SIGNAL(triggered(bool)), this, SLOT(reset()));
    QObject::connect(ui->actionQuit, SIGNAL(triggered(bool)), qApp, SLOT(quit()));
    QObject::connect(ui->actionAbout, SIGNAL(triggered(bool)), this, SLOT(showAbout()));

    QSignalMapper* map = new QSignalMapper(this);
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            QString cellName = QString("cell%1%2").arg(row).arg(col);
            Cell* cell = this->findChild<Cell*>(cellName);
            Q_ASSERT(cell != nullptr);
            Q_ASSERT(cell->row() == row && cell->col() == col);

            m_board[row][col] = cell;

            int id = row * 8 + col;
            map->setMapping(cell, id);
            QObject::connect(cell, SIGNAL(clicked(bool)), map, SLOT(map()));
            QObject::connect(cell, SIGNAL(mouseOver(bool)), this, SLOT(updateSelectables(bool)));
        }
    }
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QObject::connect(map, SIGNAL(mapped(int)), this, SLOT(play(int)));
#else
    QObject::connect(map, SIGNAL(mappedInt(int)), this, SLOT(play(int)));
#endif

    // When the turn ends, switch the player.
    QObject::connect(this, SIGNAL(turnEnded()), this, SLOT(switchPlayer()));

    this->reset();

    this->adjustSize();
    this->setFixedSize(this->size());
}

Catch::~Catch() {
    delete ui;
}

void Catch::play(int id) {
    Cell* cell = m_board[id / 8][id % 8];
    Cell* cell1;
    if(m_player->orientation()==Player::Horizontal){
        if (cell->col()< 7){
            cell1=m_board[cell->row()][cell->col()+1];
        }
    } else {
        if (cell->row() < 7){
            cell1=m_board[cell->row()+1][cell->col()];
        }}

    if (cell == nullptr || !cell->isSelectable())
        return;

    cell->setState(Cell::Blocked);
    cell1->setState(Cell::Blocked);
    bool a=false;
    // verifica se tem jogada
    if(m_player->orientation()==Player::Horizontal){
        if (cell->col()< 7){
            cell1=m_board[cell->row()][cell->col()+1];
            a=true;
        }
        else{
            a=false;
        }
    } else {
        if (cell->row() < 7){
            cell1=m_board[cell->row()+1][cell->col()];
            a=true;
        }else{
            a=false;
        }
    }
    // capturando casas
    const int BOARD_SIZE = 8;

    QSet<QPair<int, int>> checada;



    // Percorre a matriz de células
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            if(a==false){
                break;
            }
            Cell* cell = m_board[row][col];

            // Verifica se a célula está vazia e não foi verificada antes
            if (cell->isEmpty() && !checada.contains(qMakePair(row, col))) {
                QList<Cell*> cluster;
                QList<Cell*> todos;

                todos.append(cell);

                // Busca em largura para encontrar o cluster
                while (!todos.isEmpty()) {
                    Cell* celula = todos.last();
                    todos.removeLast();

                    int rowAtual = celula->row();
                    int colAtual = celula->col();

                    if (rowAtual >= 0 && rowAtual < BOARD_SIZE && colAtual >= 0 && colAtual < BOARD_SIZE && m_board[rowAtual][colAtual]->isEmpty() && !checada.contains(qMakePair(rowAtual, colAtual))) {
                        checada.insert(qMakePair(rowAtual, colAtual));

                        cluster.append(celula);

                        // Adiciona os vizinhos ao conjunto de células a serem verificadas
                        if (rowAtual < BOARD_SIZE - 1){
                            todos.append(m_board[rowAtual + 1][colAtual]);
                        }if (rowAtual > 0){
                            todos.append(m_board[rowAtual - 1][colAtual]);
                        }if (colAtual > 0){
                            todos.append(m_board[rowAtual][colAtual - 1]);
                        }if (colAtual < BOARD_SIZE - 1){
                            todos.append(m_board[rowAtual][colAtual + 1]);
                        }
                    }
                }

                // Atribui um jogador às células do cluster (vermelho ou azul)
                if (cluster.size() > 0 && cluster.size() <= 3) {
                    for (Cell* clusterCell : cluster) {
                        clusterCell->setPlayer(m_player);
                        m_player->incrementCount();
                    }

                }
            }
        }
    }

    emit turnEnded();
    fimdojogo();

}

void Catch::switchPlayer() {
    // Switch the player.
    m_player = m_player->other();

    // Finally, update the status bar.
    this->updateStatusBar();
}

void Catch::fimdojogo() {
    bool jogadas= false;
    // resetando
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            Cell* cell = m_board[row][col];
            if(m_player->orientation()==Player::Horizontal){
                if (cell->col()< 7){
                    Cell* cell1 = m_board[row][col+1];
                    if (cell->isEmpty() && cell1->isEmpty()) {
                        jogadas = true;
                    }
                }
            }else{
                if (cell->row()< 7){
                    Cell* cell1 = m_board[row+1][col];
                    if (cell->isEmpty() && cell1->isEmpty()) {
                        jogadas = true;
                    }
                }


            }

        }
    }


    if (!jogadas) {
        if (m_player->count()== m_player->other()->count()) {
                    QMessageBox::information(this, tr("Game Over"), tr("Empatou!"));
                } else if (m_player->count() < m_player->other()->count()) {
                    QMessageBox::information(this, tr("Game Over"), tr("%1 ganhou!").arg(m_player->other()->name()));
                } else {
                    QMessageBox::information(this, tr("Game Over"), tr("%1 ganhou!").arg(m_player->name()));
                }

        // Reset the game
        reset();
    }

}


void Catch::reset() {
    // Reset board.
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            Cell* cell = m_board[row][col];
            cell->reset();
        }
    }

    // Reset the players.
    Player* red = Player::player(Player::Red);
    red->reset();

    Player* blue = Player::player(Player::Blue);
    blue->reset();

    m_player = red;

    // Finally, update the status bar.
    this->updateStatusBar();
}

void Catch::showAbout() {
    QMessageBox::information(this, tr("Sobre"), tr("Catch\n\nCustodio Junio, Laura CAmpos - Lauracampos02@icloud.com - Custodiojunio23@gmail.com"));
}

void Catch::updateSelectables(bool over) {
    Cell* cell = qobject_cast<Cell*>(QObject::sender());
    Cell* cell1;
    Q_ASSERT(cell != nullptr);
    bool a=false;

    if(m_player->orientation()==Player::Horizontal){
        if (cell->col()< 7){
        cell1=m_board[cell->row()][cell->col()+1];
        a=true;
        }
     } else {
        if (cell->row() < 7){
        cell1=m_board[cell->row()+1][cell->col()];
        a=true;
    }}
    if (over && a) {
        if (cell->isEmpty() ){
                if (cell1->isEmpty()){
                cell->setState(Cell::Selectable);
                cell1->setState(Cell::Selectable);
                }
            }

    } else {
        if (cell->isSelectable()){
            cell->setState(Cell::Empty);
            cell1->setState(Cell::Empty);
        }
    }
}

void Catch::updateStatusBar() {
    ui->statusbar->showMessage(tr("Vez do %1 (%2 a %3)")
        .arg(m_player->name()).arg(m_player->count()).arg(m_player->other()->count()));
}


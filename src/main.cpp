#include "application_ui.h"
#include "SDL2_gfxPrimitives.h"
#include <vector>
#include <list>
#include <map>
#include <queue>
#include <algorithm>
#include <iostream>

#define EPSILON 0.0001f

struct Coords
{
    int x, y;

    bool operator==(const Coords& other)
    {
        return x == other.x and y == other.y;
    }
    bool operator!=(const Coords& other)
    {
        return x != other.x or y != other.y;
    }
};

struct Segment
{
    Coords p1, p2;

    bool operator==(const Segment& other)
    {
        return p1 == other.p1 and p2 == other.p2
        and p1 == other.p2 and p2 == other.p1;
    }
    bool operator!=(const Segment& other)
    {
        return p1 != other.p1 or p2 != other.p2
        or p1 != other.p2 or p2 != other.p1;
    }
};

struct Triangle
{
    Coords p1, p2, p3;
    Segment s1{p1, p2}, s2{p2, p3}, s3{p3, p1};
    bool complet=false;

    bool operator==(const Triangle& other)
    {
        return p1 == other.p1 and p2 == other.p2 and p3 == other.p3;
    }
};

struct Application
{
    int width, height;
    Coords focus{100, 100};

    std::vector<Coords> points;
    std::vector<Triangle> triangles;
};

bool compareCoords(Coords point1, Coords point2)
{
    if (point1.x == point2.x)
        return point1.y < point2.y;
    return point1.x < point2.x;
}

void drawPoints(SDL_Renderer *renderer, const std::vector<Coords> &points)
{
    for (std::size_t i = 0; i < points.size(); i++)
    {
        filledCircleRGBA(renderer, points[i].x, points[i].y, 3, 240, 240, 23, SDL_ALPHA_OPAQUE);
    }
}

void drawSegments(SDL_Renderer *renderer, const std::vector<Segment> &segments)
{
    for (std::size_t i = 0; i < segments.size(); i++)
    {
        lineRGBA(
            renderer,
            segments[i].p1.x, segments[i].p1.y,
            segments[i].p2.x, segments[i].p2.y,
            240, 240, 20, SDL_ALPHA_OPAQUE);
    }
}

void drawTriangles(SDL_Renderer *renderer, const std::vector<Triangle> &triangles)
{
    for (std::size_t i = 0; i < triangles.size(); i++)
    {
        const Triangle& t = triangles[i];
        trigonRGBA(
            renderer,
            t.p1.x, t.p1.y,
            t.p2.x, t.p2.y,
            t.p3.x, t.p3.y,
            0, 240, 160, SDL_ALPHA_OPAQUE
        );
    }
}

void draw(SDL_Renderer *renderer, const Application &app)
{
    /* Remplissez cette fonction pour faire l'affichage du jeu */
    int width, height;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    drawPoints(renderer, app.points);
    drawTriangles(renderer, app.triangles);
}

/*
   Détermine si un point se trouve dans un cercle définit par trois points
   Retourne, par les paramètres, le centre et le rayon
*/
bool CircumCircle(
    float pX, float pY,
    float x1, float y1, float x2, float y2, float x3, float y3,
    float *xc, float *yc, float *rsqr
)
{
    float m1, m2, mx1, mx2, my1, my2;
    float dx, dy, drsqr;
    float fabsy1y2 = fabs(y1 - y2);
    float fabsy2y3 = fabs(y2 - y3);

    /* Check for coincident points */
    if (fabsy1y2 < EPSILON && fabsy2y3 < EPSILON)
        return (false);

    if (fabsy1y2 < EPSILON)
    {
        m2 = -(x3 - x2) / (y3 - y2);
        mx2 = (x2 + x3) / 2.0;
        my2 = (y2 + y3) / 2.0;
        *xc = (x2 + x1) / 2.0;
        *yc = m2 * (*xc - mx2) + my2;
    }
    else if (fabsy2y3 < EPSILON)
    {
        m1 = -(x2 - x1) / (y2 - y1);
        mx1 = (x1 + x2) / 2.0;
        my1 = (y1 + y2) / 2.0;
        *xc = (x3 + x2) / 2.0;
        *yc = m1 * (*xc - mx1) + my1;
    }
    else
    {
        m1 = -(x2 - x1) / (y2 - y1);
        m2 = -(x3 - x2) / (y3 - y2);
        mx1 = (x1 + x2) / 2.0;
        mx2 = (x2 + x3) / 2.0;
        my1 = (y1 + y2) / 2.0;
        my2 = (y2 + y3) / 2.0;
        *xc = (m1 * mx1 - m2 * mx2 + my2 - my1) / (m1 - m2);
        if (fabsy1y2 > fabsy2y3)
        {
            *yc = m1 * (*xc - mx1) + my1;
        }
        else
        {
            *yc = m2 * (*xc - mx2) + my2;
        }
    }

    dx = x2 - *xc;
    dy = y2 - *yc;
    *rsqr = dx * dx + dy * dy;

    dx = pX - *xc;
    dy = pY - *yc;
    drsqr = dx * dx + dy * dy;

    return ((drsqr - *rsqr) <= EPSILON ? true : false);
}

void construitDelaunay(Application &app) {
    std::sort(app.points.begin(), app.points.end(), compareCoords);
    app.triangles.clear();
    app.triangles.push_back({{-1000, -1000}, {500, 3000}, {1500, -1000}});
    // app.triangles.front().makeLimite();
    for (Coords& p: app.points) {
        std::vector<Segment> segments;
        std::vector<Segment*> bad_segments;
        std::vector<Triangle*> bad_triangles;
        
        for (Triangle& t: app.triangles) {
            float xc, yc, rsqr;
            Triangle* tri = &t;
            bool inCircle = CircumCircle(p.x, p.y,
                    t.p1.x, t.p1.y, t.p2.x, t.p2.y, t.p3.x, t.p3.y,
                    &xc, &yc, &rsqr);
            if (inCircle) {
                segments.push_back(Segment{t.p1, t.p2});
                segments.push_back(Segment{t.p2, t.p3});
                segments.push_back(Segment{t.p3, t.p1});
                bad_triangles.push_back(tri);
            }
            
        }

        /*for (Triangle* bt1: bad_triangles) {
            for (Triangle* bt2: bad_triangles) {
                if (&bt1 == &bt2) continue;
                std::cout << "wooo!" << std::endl;
                if (bt1->s1 != bt2->s1
                 && bt1->s1 != bt2->s2
                 && bt1->s1 != bt2->s3) {
                    std::cout << "hey!" << std::endl;
                    segments.push_back(bt1->s1);
                }
                if (bt1->s2 != bt2->s1
                 && bt1->s2 != bt2->s2
                 && bt1->s2 != bt2->s3) {
                    std::cout << "hey!" << std::endl;
                    segments.push_back(bt1->s2);
                }
                if (bt1->s3 != bt2->s1
                 && bt1->s3 != bt2->s2
                 && bt1->s3 != bt2->s3) {
                    std::cout << "hey!" << std::endl;
                    segments.push_back(bt1->s3);
                }

            }
        }*/

        for (Segment& s1: segments) {
            for (Segment& s2: segments) {
                if (&s1 == &s2) continue;
                if (s1.p1 == s2.p2 && s1.p2 == s2.p1) {
                    Segment* seg1 = &s1;
                    Segment* seg2 = &s2;
                    bad_segments.push_back(seg1);
                    bad_segments.push_back(seg2);
                }
            }
        }
        /*std::cout << "bad_segments AVANT: " << bad_segments.size() << std::endl;
        bad_segments.erase(unique(bad_segments.begin(), bad_segments.end()), bad_segments.end());
        std::cout << "bad_segments APRES: " << bad_segments.size() << std::endl;*/

        for (Triangle* bt: bad_triangles) {
            auto to_delete = std::remove(app.triangles.begin(), app.triangles.end(), *bt);
            app.triangles.erase(to_delete, app.triangles.end());
        }

        /*for (Segment* elem: bad_segments) {
            auto to_delete = std::remove(segments.begin(), segments.end(), *elem);
            segments.erase(to_delete, segments.end());
        }*/

        for (Segment& s: segments) {
            app.triangles.push_back(Triangle {s.p1, s.p2, p});
        }
    }
}

void construitVoronoi(Application &app)
{
    app.triangles.clear();

    // Tri des points selon leurs coordonnées y croissantes
    std::sort(app.points.begin(), app.points.end(), compareCoords);

    // Création des triangles à partir des points générés
    for (std::size_t i = 0; i < app.points.size(); i++)
    {
        for (std::size_t j = i + 1; j < app.points.size(); j++)
        {
            for (std::size_t k = j + 1; k < app.points.size(); k++)
            {
                const Coords &p1 = app.points[i];
                const Coords &p2 = app.points[j];
                const Coords &p3 = app.points[k];

                Triangle triangle{p1, p2, p3};
                app.triangles.push_back(triangle);
            }
        }
    }
}

bool handleEvent(Application &app)
{
    /* Remplissez cette fonction pour gérer les inputs utilisateurs */
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
            return false;
        else if (e.type == SDL_WINDOWEVENT_RESIZED)
        {
            app.width = e.window.data1;
            app.height = e.window.data1;
        }
        else if (e.type == SDL_MOUSEWHEEL)
        {
        }
        else if (e.type == SDL_MOUSEBUTTONUP)
        {
            if (e.button.button == SDL_BUTTON_RIGHT)
            {
                app.focus.x = e.button.x;
                app.focus.y = e.button.y;
                app.points.clear();
            }
            else if (e.button.button == SDL_BUTTON_LEFT)
            {
                app.focus.y = 0;
                app.points.push_back(Coords{e.button.x, e.button.y});
                construitDelaunay(app);
            }
        }
    }
    return true;
}

int main()
{
    SDL_Window *gWindow;
    SDL_Renderer *renderer;
    Application app{720, 720, Coords{0, 0}};
    bool is_running = true;

    // Creation de la fenetre
    gWindow = init("Awesome Voronoi", 720, 720);

    if (!gWindow)
    {
        SDL_Log("Failed to initialize!\n");
        exit(1);
    }

    renderer = SDL_CreateRenderer(gWindow, -1, 0); // SDL_RENDERER_PRESENTVSYNC

    /*  GAME LOOP  */
    while (true)
    {
        // INPUTS
        is_running = handleEvent(app);
        if (!is_running)
            break;

        // EFFACAGE FRAME
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // DESSIN
        draw(renderer, app);

        // VALIDATION FRAME
        SDL_RenderPresent(renderer);

        // PAUSE en ms
        SDL_Delay(1000 / 30);
    }

    // Free resources and close SDL
    close(gWindow, renderer);

    return 0;
}

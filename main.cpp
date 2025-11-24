#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <limits>
#include <algorithm>
// Make sure these header files exist in your project
#include "Perceptron.h"
#include "FeatureExtractor.h"

#define ID_BTN_TRAIN   1
#define ID_BTN_USE     2
#define ID_EDIT_INPUT  100
#define ID_BTN_ACTION  101
#define ID_FRAME       102

// ---------------- App State ----------------
enum AppState { STATE_MENU, STATE_TRAIN, STATE_USE };
static AppState gState = STATE_MENU;

// ---------------- UI Globals ----------------
HBRUSH gBtnBrush = NULL;     // black for buttons
HBRUSH gLogBrush = NULL;     // black brush for log edit
HWND   gEditInput = NULL;    // input edit (hidden until training)
HWND   gActionBtn = NULL;    // "Action" button
HWND   gFrameBox = NULL;    // etched frame rect for log
HWND   gLogEdit = NULL;      // scrollable log output
HWND   gTrainBtn = NULL;     // Train Mode button
HWND   gUseBtn = NULL;       // Use Mode button
std::wstring gLogBuffer;     // printed text (replaces std::cout)

// Add a global brush (so it doesn’t get destroyed right away)
HBRUSH gLogBgBrush;


// ---------------- Train Flow Globals ----------------
int gTrainStep = -1;
int gExpectedInputs = 0;
int gCurrentSample = 0;
int gEpochs = 10;

// ---------------- Use Flow Globals ----------------
int gUseStep = -1;

std::vector<std::vector<double>> gX;
std::vector<int>                 gY;
std::vector<std::string>         gKeywords;
Perceptron* gPerceptron = nullptr;

// ---------------- Helpers ----------------
static void setGreenText(HDC hdc) {
    SetTextColor(hdc, RGB(0, 255, 0));
    SetBkColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
}

static void AppendLog(const std::wstring& s, HWND hwnd) {
    gLogBuffer += s;
    gLogBuffer += L"\r\n";

    // Update the scrollable edit control
    if (gLogEdit) {
        SetWindowText(gLogEdit, gLogBuffer.c_str());
        // Auto-scroll to bottom
        int len = GetWindowTextLength(gLogEdit);
        SendMessage(gLogEdit, EM_SETSEL, len, len);
        SendMessage(gLogEdit, EM_SCROLLCARET, 0, 0);
    }

    InvalidateRect(hwnd, NULL, TRUE);
}

static std::wstring Widen(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

static std::vector<std::string> buildKeywordList() {
    // If you have your own list, replace this with the original.
    return {
        "free","win","money","offer","click","buy","urgent",
        "reward","account","verify","login","pin","selected",
        "limited","now","risk","credit","deal","bonus","gift"
    };
}

// Paint the log text inside gFrameBox, aligned to that child's rectangle
static void PaintLogInFrame(HWND hwnd, HDC hdc) {
    if (!gFrameBox) return;
    RECT rWin;
    GetWindowRect(gFrameBox, &rWin);
    MapWindowPoints(NULL, hwnd, (LPPOINT)&rWin, 2); // convert to parent client coords
    RECT rc = rWin;
    InflateRect(&rc, -8, -8); // padding inside the frame
    DrawText(hdc, gLogBuffer.c_str(), -1, &rc, DT_LEFT | DT_TOP | DT_NOPREFIX);
}

// Forward declarations
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void StartUseFlow(HWND hwnd);
static void HandleUseInput(HWND hwnd);
static void StartUseFlow(HWND hwnd) {
    // reset UI log + state
    gLogBuffer.clear();

    if (gPerceptron) {
        delete gPerceptron;
        gPerceptron = nullptr;
    }
    gKeywords = buildKeywordList();
    gPerceptron = new Perceptron((int)gKeywords.size(), 0.001);

    AppendLog(L"Load existing model? (y/n): ", hwnd);
    gUseStep = 0; // expecting y/n
    SetWindowText(gEditInput, L"");
    SetFocus(gEditInput);
}

// Handles the Action button click during use mode
static void HandleUseInput(HWND hwnd) {
    if (gUseStep < 0) return;

    wchar_t wbuf[512];
    GetWindowText(gEditInput, wbuf, 512);
    std::wstring inputW = wbuf;
    std::string input(inputW.begin(), inputW.end());
    SetWindowText(gEditInput, L""); // clear after read

    switch (gUseStep) {
    case 0: { // Load model? (y/n)
        char loadChoice = input.empty() ? 'n' : input[0];
        if (loadChoice == 'y' || loadChoice == 'Y') {
            AppendLog(L"Filename [model.txt]: ", hwnd);
            gUseStep = 1;
        }
        else {
            AppendLog(L"Using fresh random weights.", hwnd);
            AppendLog(L"File to classify: ", hwnd);
            gUseStep = 2;
        }
    } break;

    case 1: { // filename to load
        std::string path = input.empty() ? "model.txt" : input;
        if (gPerceptron->loadModel(path)) {
            AppendLog(L"Loaded '" + Widen(path) + L"'.", hwnd);
        }
        else {
            AppendLog(L"Could not load '" + Widen(path) + L"', using fresh random weights.", hwnd);
        }
        AppendLog(L"File to classify: ", hwnd);
        gUseStep = 2;
    } break;

    case 2: { // file to classify
        // Check file existence
        std::ifstream f(input);
        if (!f) {
            AppendLog(L"Could not open '" + inputW + L"'. Please try again.", hwnd);
            AppendLog(L"File to classify: ", hwnd);
            return;
        }

        // Extract features and classify
        auto feats = extractFeatures(input, gKeywords);
        int pred = gPerceptron->predict(feats);
        double sc = gPerceptron->score(feats);

        // Display results
        std::wstringstream results;
        results << L"\nFeatures: ";
        for (size_t i = 0; i < feats.size(); ++i) {
            results << feats[i];
            if (i + 1 < feats.size()) results << L" ";
        }
        results << L"\nScore: " << sc
            << L"  => Prediction: " << (pred ? L"SPAM" : L"HAM");
        AppendLog(results.str(), hwnd);

        AppendLog(L"\nClassify another file? (y/n): ", hwnd);
        gUseStep = 3;
    } break;

    case 3: { // classify another?
        char ch = input.empty() ? 'n' : input[0];
        if (ch == 'y' || ch == 'Y') {
            AppendLog(L"File to classify: ", hwnd);
            gUseStep = 2;
        }
        else {
            AppendLog(L"Use mode complete.", hwnd);
            gUseStep = -1;
        }
    } break;
    }
}

// ---------------- Train Flow State Machine ----------------
static void StartTrainFlow(HWND hwnd) {
    // reset UI log + state
    gLogBuffer.clear();
    gX.clear();
    gY.clear();
    gExpectedInputs = 0;
    gCurrentSample = 0;
    gEpochs = 10;

    if (gPerceptron) {
        delete gPerceptron;
        gPerceptron = nullptr;
    }
    gKeywords = buildKeywordList();
    gPerceptron = new Perceptron((int)gKeywords.size(), 0.001);

    AppendLog(L"Load existing model before training? (y/n): ", hwnd);
    gTrainStep = 0; // expecting y/n
    SetWindowText(gEditInput, L"");
    SetFocus(gEditInput);
}

// Handles the Action button click during training
static void HandleTrainInput(HWND hwnd) {
    if (gTrainStep < 0) return;

    wchar_t wbuf[512];
    GetWindowText(gEditInput, wbuf, 512);
    std::wstring inputW = wbuf;
    std::string  input(inputW.begin(), inputW.end());
    SetWindowText(gEditInput, L""); // clear after read

    switch (gTrainStep) {
    case 0: { // Load model? (y/n)
        char loadChoice = input.empty() ? 'n' : input[0];
        if (loadChoice == 'y' || loadChoice == 'Y') {
            AppendLog(L"Filename [model.txt]: ", hwnd);
            gTrainStep = 1;
        }
        else {
            AppendLog(L"How many training samples? ", hwnd);
            gTrainStep = 2;
        }
    } break;

    case 1: { // filename to load
        std::string path = input.empty() ? "model.txt" : input;
        if (gPerceptron->loadModel(path)) {
            AppendLog(L"Loaded model from '" + Widen(path) + L"'. Training will continue...", hwnd);
        }
        else {
            AppendLog(L"Could not load '" + Widen(path) + L"'. Starting a new model.", hwnd);
        }
        AppendLog(L"How many training samples? ", hwnd);
        gTrainStep = 2;
    } break;

    case 2: { // number of samples
        int N = 0;
        try {
            N = std::stoi(input);
        }
        catch (...) {
            N = 0;
        }
        if (N <= 0) {
            AppendLog(L"Invalid number.", hwnd);
            gTrainStep = -1;
            return;
        }
        gX.clear();
        gY.clear();
        gX.reserve(N);
        gY.reserve(N);
        gExpectedInputs = N;
        gCurrentSample = 0;

        std::wstringstream ss;
        ss << L"Sample " << (gCurrentSample + 1) << L" file path: ";
        AppendLog(ss.str(), hwnd);
        gTrainStep = 3;
    } break;

    case 3: { // file path for sample
        // Check file existence
        std::ifstream f(input);
        if (!f) {
            AppendLog(L"Could not open '" + inputW + L"'. Skipping this sample.", hwnd);
            gCurrentSample++;
            if (gCurrentSample < gExpectedInputs) {
                std::wstringstream ss;
                ss << L"Sample " << (gCurrentSample + 1) << L" file path: ";
                AppendLog(ss.str(), hwnd);
                gTrainStep = 3;
            }
            else {
                if (gX.empty()) {
                    AppendLog(L"No valid training samples provided. Exiting.", hwnd);
                    gTrainStep = -1;
                }
                else {
                    AppendLog(L"Epochs (default 10): ", hwnd);
                    gTrainStep = 5;
                }
            }
            return;
        }

        // Extract features
        auto feats = extractFeatures(input, gKeywords);
        gX.push_back(std::move(feats));

        AppendLog(L"Label (1 = spam, 0 = ham): ", hwnd);
        gTrainStep = 4;
    } break;

    case 4: { // label
        int label = 0;
        try {
            label = std::stoi(input);
        }
        catch (...) {
            label = 0;
        }
        gY.push_back(label);

        gCurrentSample++;
        if (gCurrentSample < gExpectedInputs) {
            std::wstringstream ss;
            ss << L"Sample " << (gCurrentSample + 1) << L" file path: ";
            AppendLog(ss.str(), hwnd);
            gTrainStep = 3;
        }
        else {
            if (gX.empty()) {
                AppendLog(L"No valid training samples provided. Exiting.", hwnd);
                gTrainStep = -1;
            }
            else {
                AppendLog(L"Epochs (default 10): ", hwnd);
                gTrainStep = 5;
            }
        }
    } break;

    case 5: { // epochs
        if (!input.empty()) {
            int e = 10;
            try {
                e = std::stoi(input);
            }
            catch (...) {
                e = 10;
            }
            gEpochs = (std::max)(1, e);
        }

        // ---- TRAIN LOOP (exact same prints as console) ----
        for (int e = 0; e < gEpochs; ++e) {
            std::wstringstream se;
            se << L"\n--- Epoch " << (e + 1) << L" ---";
            AppendLog(se.str(), hwnd);

            for (size_t i = 0; i < gX.size(); ++i) {
                std::wstringstream ss;
                ss << L"Sample #" << (i + 1) << L":\n";
                ss << L"  Input: ";
                for (auto v : gX[i]) ss << v << L" ";

                // before update
                double yhat_before = gPerceptron->score(gX[i]);   // this is "score"
                int    pred_before = gPerceptron->predict(gX[i]);
                int    y = gY[i];

                // gradient wrt yhat for squared loss 0.5 * (yhat - y)^2
                double grad = (yhat_before - y);
                double loss = 0.5 * grad * grad;

                ss << L"\n  Score(before): " << yhat_before
                    << L" Predicted: " << pred_before
                    << L" Expected: " << y
                    << L" Error(0/1): " << (y - pred_before)
                    << L"  Grad(yhat-y): " << grad
                    << L"  Loss: " << loss;
                AppendLog(ss.str(), hwnd);

                // train step (this uses grad internally)
                gPerceptron->train(gX[i], gY[i]);

                // after
                double yhat_after = gPerceptron->score(gX[i]);
                std::wstringstream ss2;
                ss2 << L"  Score(after): " << yhat_after << L"\n";
                ss2 << L"  Weights: ";
                const auto& W = gPerceptron->getWeights();
                for (auto w : W) ss2 << w << L" ";
                ss2 << L" Bias: " << gPerceptron->getBias() << L"\n";
                AppendLog(ss2.str(), hwnd);
            }
        }


        // final prints (exact)
        {
            std::wstringstream sf;
            sf << L"\nTrained! Weights: ";
            const auto& W = gPerceptron->getWeights();
            for (auto w : W) sf << w << L" ";
            sf << L"  Bias: " << gPerceptron->getBias();
            AppendLog(sf.str(), hwnd);
        }

        AppendLog(L"\nTraining set predictions:", hwnd);
        for (size_t i = 0; i < gX.size(); ++i) {
            double yhat = gPerceptron->score(gX[i]);
            int    pred = gPerceptron->predict(gX[i]);
            int    y = gY[i];
            double grad = (yhat - y);

            std::wstringstream sp;
            sp << L"  #" << (i + 1)
                << L" expected=" << y
                << L" predicted=" << pred
                << L"  score=" << yhat
                << L"  grad(yhat-y)=" << grad;
            AppendLog(sp.str(), hwnd);
        }


        AppendLog(L"\nSave model? (y/n): ", hwnd);
        gTrainStep = 6;
    } break;

    case 6: { // save choice
        char ch = input.empty() ? 'n' : input[0];
        if (ch == 'y' || ch == 'Y') {
            AppendLog(L"Filename [model.txt]: ", hwnd);
            gTrainStep = 7;
        }
        else {
            AppendLog(L"Training complete.", hwnd);
            gTrainStep = -1;
        }
    } break;

    case 7: { // save filename
        std::string path = input.empty() ? "model.txt" : input;
        if (gPerceptron->saveModel(path)) {
            AppendLog(L"Saved to '" + Widen(path) + L"'.", hwnd);
        }
        else {
            AppendLog(L"Failed to save '" + Widen(path) + L"'.", hwnd);
        }
        gTrainStep = -1;
    } break;
    }
}

static void SwitchState(HWND hwnd, AppState newState) {
    gState = newState;

    // Show/hide training controls
    if (gState == STATE_TRAIN) {
        // Hide main menu buttons
        ShowWindow(gTrainBtn, SW_HIDE);
        ShowWindow(gUseBtn, SW_HIDE);

        // Show training controls
        ShowWindow(gEditInput, SW_SHOW);
        ShowWindow(gActionBtn, SW_SHOW);
        ShowWindow(gFrameBox, SW_SHOW);
        ShowWindow(gLogEdit, SW_SHOW);
        StartTrainFlow(hwnd);
    }
    else if (gState == STATE_USE) {
        // Hide main menu buttons
        ShowWindow(gTrainBtn, SW_HIDE);
        ShowWindow(gUseBtn, SW_HIDE);

        // Show use controls
        ShowWindow(gEditInput, SW_SHOW);
        ShowWindow(gActionBtn, SW_SHOW);
        ShowWindow(gFrameBox, SW_SHOW);
        ShowWindow(gLogEdit, SW_SHOW);
        StartUseFlow(hwnd);
    }
    else {
        // Back to menu - show main buttons, hide everything else
        ShowWindow(gTrainBtn, SW_SHOW);
        ShowWindow(gUseBtn, SW_SHOW);
        ShowWindow(gEditInput, SW_HIDE);
        ShowWindow(gActionBtn, SW_HIDE);
        ShowWindow(gFrameBox, SW_HIDE);
        ShowWindow(gLogEdit, SW_HIDE);
        gLogBuffer.clear();
        SetWindowText(gLogEdit, L"");
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // Inside WinMain (or init code), create the brush
    gLogBgBrush = CreateSolidBrush(RGB(20, 20, 20)); // very dark gray/black

    const wchar_t CLASS_NAME[] = L"PerceptronClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); // keep background BLACK

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Perceptron.SsH",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        gBtnBrush = CreateSolidBrush(RGB(0, 0, 0)); // black button fill
        gLogBrush = CreateSolidBrush(RGB(0, 0, 0)); // black background for log edit

        // Owner-draw main buttons
        gTrainBtn = CreateWindow(L"BUTTON", L"Train Mode",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            210, 400, 150, 50, hwnd, (HMENU)ID_BTN_TRAIN,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        gUseBtn = CreateWindow(L"BUTTON", L"Use Mode",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            420, 400, 150, 50, hwnd, (HMENU)ID_BTN_USE,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        // Training UI (hidden until Train state)
        gEditInput = CreateWindow(L"EDIT", L"",
            WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            50, 500, 500, 25, hwnd, (HMENU)ID_EDIT_INPUT,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        gActionBtn = CreateWindow(L"BUTTON", L"Action",
            WS_CHILD | WS_TABSTOP | BS_OWNERDRAW,
            570, 500, 100, 25, hwnd, (HMENU)ID_BTN_ACTION,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        gFrameBox = CreateWindow(L"STATIC", NULL,
            WS_CHILD | SS_ETCHEDFRAME,
            30, 30, 720, 450, hwnd, (HMENU)ID_FRAME,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        // Create scrollable log edit control
        gLogEdit = CreateWindow(L"EDIT", L"",
            WS_CHILD | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            40, 40, 700, 430, hwnd, NULL,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        // Set font for the log edit
        HFONT hLogFont = CreateFont(
            14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, FF_MODERN, L"Consolas");
        SendMessage(gLogEdit, WM_SETFONT, (WPARAM)hLogFont, TRUE);

        ShowWindow(gEditInput, SW_HIDE);
        ShowWindow(gActionBtn, SW_HIDE);
        ShowWindow(gFrameBox, SW_HIDE);
        ShowWindow(gLogEdit, SW_HIDE);
    }
    break;

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        if (id == ID_BTN_TRAIN) {
            SwitchState(hwnd, STATE_TRAIN);
        }
        else if (id == ID_BTN_USE) {
            SwitchState(hwnd, STATE_USE);
        }
        else if (id == ID_BTN_ACTION) {
            if (gState == STATE_TRAIN) {
                HandleTrainInput(hwnd);
            }
            else if (gState == STATE_USE) {
                HandleUseInput(hwnd);
            }
        }
    }
    break;

    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
        if (pDIS->CtlID == ID_BTN_TRAIN || pDIS->CtlID == ID_BTN_USE || pDIS->CtlID == ID_BTN_ACTION)
        {
            // Fill black
            FillRect(pDIS->hDC, &pDIS->rcItem, gBtnBrush);

            // Green text
            SetTextColor(pDIS->hDC, RGB(0, 255, 0));
            SetBkMode(pDIS->hDC, TRANSPARENT);

            // Green outline
            HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
            HGDIOBJ oldPen = SelectObject(pDIS->hDC, hPen);
            HGDIOBJ oldBrush = SelectObject(pDIS->hDC, GetStockObject(NULL_BRUSH));
            Rectangle(pDIS->hDC,
                pDIS->rcItem.left, pDIS->rcItem.top,
                pDIS->rcItem.right, pDIS->rcItem.bottom);
            SelectObject(pDIS->hDC, oldPen);
            SelectObject(pDIS->hDC, oldBrush);
            DeleteObject(hPen);

            // Button text
            const wchar_t* text = L"";
            if (pDIS->CtlID == ID_BTN_TRAIN) text = L"Train Mode";
            else if (pDIS->CtlID == ID_BTN_USE) text = L"Use Mode";
            else if (pDIS->CtlID == ID_BTN_ACTION) text = L"Action";

            RECT rc = pDIS->rcItem;
            if (pDIS->itemState & ODS_SELECTED) OffsetRect(&rc, 1, 1);
            DrawText(pDIS->hDC, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }
    break;

    case WM_CTLCOLOREDIT:
    {
        HDC hdcEdit = (HDC)wParam;
        HWND hEdit = (HWND)lParam;

        // Only apply colors to our log edit control
        if (hEdit == gLogEdit || hEdit == gEditInput) {
            SetTextColor(hdcEdit, RGB(0, 255, 0)); // green text
            SetBkColor(hdcEdit, RGB(0, 0, 0));     // black background
            return (INT_PTR)gLogBrush;
        }

        // Default handling for other edit controls
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    break;

    case WM_CTLCOLORSTATIC: // for read-only EDIT
    {
        HDC hdcEdit = (HDC)wParam;
        HWND hEdit = (HWND)lParam;

        if (hEdit == gLogEdit) {
            // Set text color to green
            SetTextColor(hdcEdit, RGB(0, 255, 0));
            // Set background color to match our brush
            SetBkColor(hdcEdit, RGB(20, 20, 20));
            return (INT_PTR)gLogBgBrush; // return our custom brush
        }
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Black background already handled by class brush, but we can repaint explicitly
        RECT client;
        GetClientRect(hwnd, &client);
        HBRUSH black = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &client, black);
        DeleteObject(black);

        setGreenText(hdc);

        // Monospace for ASCII/log
        HFONT hFont = CreateFont(
            16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, FF_MODERN, L"Consolas");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        if (gState == STATE_MENU) {
            const wchar_t* asciiArt[] = {
                L"        ...                  ...        ",
                L"       .cdl.                .:dl.       ",
                L"        .,,,,,.          .',,,,'.       ",
                L"          .;dd:.        .'ldc.          ",
                L"       .,ccldolcc::cccccccodlcc:.       ",
                L"     ..'cdoccclodooddoooolcccodo;..     ",
                L"    'lolodl'  'odooddoodd:. .cdoooo;.   ",
                L" .,,:odoooo:',:odooddooodc,,;ldooodc,,. ",
                L".:ddooooodooddoodoodddoddoodooddooooodc.",
                L".:do:..,ldoodooodooddooodoodoodo;..,ldl.",
                L".:do,  .cdoccccccc:ccccccccccodl'  .ldc.",
                L" :do,  .cdl'                .cdl'  .ldc.",
                L" .;,.   ';,''''''.    .'''''',,,.  .';' ",
                L"           ;doooo:.   ,ooooo:.          ",
                L"           .......    .......           "
            };
            int startX = 230;
            int startY = 50;
            int lineHeight = 18;
            for (int i = 0; i < (int)(sizeof(asciiArt) / sizeof(asciiArt[0])); ++i) {
                TextOutW(hdc, startX, startY + i * lineHeight,
                    asciiArt[i], lstrlenW(asciiArt[i]));
            }
        }
        else if (gState == STATE_TRAIN) {
            // Draw the log buffer inside the etched frame
            PaintLogInFrame(hwnd, hdc);
        }
        else if (gState == STATE_USE) {
            // Placeholder (you can add a use-mode log area if you want)
            RECT rc;
            GetClientRect(hwnd, &rc);
            DrawText(hdc, L"Use Mode not implemented Properly yet.", -1, &rc,
                DT_LEFT | DT_TOP | DT_NOPREFIX);
        }

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);

        EndPaint(hwnd, &ps);
    }
    break;

    case WM_DESTROY:
        if (gPerceptron) {
            delete gPerceptron;
            gPerceptron = nullptr;
        }
        if (gBtnBrush) {
            DeleteObject(gBtnBrush);
            gBtnBrush = NULL;
        }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
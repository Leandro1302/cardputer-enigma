#include "UiModel.h"
#include <algorithm>
namespace app {
const char* UiModel::screenName() const {
    switch(screen) {
    case Screen::Menu: return "menu";
    case Screen::Configure: return "configure";
    case Screen::Edit: return "edit";
    case Screen::Message: return "message";
    case Screen::Summary: return "summary";
    case Screen::Help: return "help";
    }
    return "unknown";
}
const char* UiModel::fieldName(int i) {
    static const char* names[] = {"Rotors L-M-R", "Initial positions", "Ring settings", "Reflector", "Plugboard", "Apply & type", "Cancel"};
    return i >= 0 && i < 7 ? names[i] : "?";
}
std::string UiModel::fieldValue(int i) const {
    if(i==0) return std::to_string(draft.rotors[0])+std::to_string(draft.rotors[1])+std::to_string(draft.rotors[2]);
    if(i==1) return enigma::letters(draft.initial);
    if(i==2) return enigma::letters(draft.rings);
    if(i==3) return std::string(1,draft.reflector);
    if(i==4) return draft.plugboard();
    return "";
}
void UiModel::resetMessage() {
    machine.reset(); input.clear(); output.clear(); history_.clear(); scroll=0;
    notice="Message and rotors reset"; ++revision;
}
void UiModel::commitEdit() {
    auto next=draft;
    bool valid=true;
    if(field==0) {
        valid=editor.size()==3;
        if(valid) for(unsigned i=0;i<3;++i) {
            if(editor[i]<'1'||editor[i]>'8') valid=false;
            next.rotors[i]=editor[i]-'0';
        }
    } else if(field==1) valid=enigma::parseLetters(editor,next.initial);
    else if(field==2) valid=enigma::parseLetters(editor,next.rings);
    else if(field==3) { valid=editor=="B"||editor=="C"; if(valid) next.reflector=editor[0]; }
    else if(field==4) valid=next.setPlugboard(editor);
    auto error=next.validate();
    if(!valid||error!=enigma::Error::None) {
        notice=valid?enigma::errorName(error):"Invalid value; edit and retry"; return;
    }
    draft=next; screen=Screen::Configure; notice="Saved in draft; choose Apply";
}
void UiModel::enter() {
    if(screen==Screen::Menu) {
        notice.clear();
        switch(selected) {
        case 0: draft=machine.config(); field=0; screen=Screen::Configure; break;
        case 1: screen=Screen::Message; break;
        case 2: screen=Screen::Summary; break;
        case 3: resetMessage(); break;
        case 4: screen=Screen::Help; break;
        }
    } else if(screen==Screen::Configure) {
        if(field==6) { screen=Screen::Menu; return; }
        if(field==5) {
            const auto e=machine.configure(draft);
            if(e==enigma::Error::None) { resetMessage(); screen=Screen::Message; }
            else notice=enigma::errorName(e);
        } else { editor=fieldValue(field); screen=Screen::Edit; notice="Backspace to edit; Enter saves"; }
    } else if(screen==Screen::Edit) commitEdit();
    else if(screen==Screen::Message) notice="Fn+Backspace: new message";
    else screen=Screen::Menu;
}
void UiModel::key(const std::string& key) {
    ++revision;
    if(key=="ESC") {
        screen=screen==Screen::Edit?Screen::Configure:Screen::Menu; notice.clear(); return;
    }
    if(key=="ENTER") { enter(); return; }
    if(key=="RESET") { resetMessage(); return; }
    if(screen==Screen::Menu && (key=="LEFT"||key=="RIGHT"||key=="UP"||key=="DOWN")) {
        const int direction=(key=="LEFT"||key=="UP")?-1:1;
        selected=(selected+direction+MenuCount)%MenuCount;
        notice.clear();
        return;
    }
    if(key=="UP"||key=="DOWN") {
        int direction=key=="UP"?-1:1;
        if(screen==Screen::Configure) field=(field+direction+7)%7;
        else if(screen==Screen::Message) {
            if(direction<0) scroll=std::min(input.size(),scroll+38);
            else scroll=scroll>38?scroll-38:0;
        }
        return;
    }
    if(key=="LEFT"||key=="RIGHT") return;
    if(key=="BACKSPACE") {
        if(screen==Screen::Edit && !editor.empty()) editor.pop_back();
        if(screen==Screen::Message && !history_.empty()) {
            machine.restore(history_.back()); history_.pop_back(); input.pop_back(); output.pop_back(); scroll=0;
        }
        notice.clear(); return;
    }
    if(key.size()!=1) return;
    char c=enigma::uppercase(key[0]);
    if(c<32||c>126) return;
    if(screen==Screen::Edit) {
        if(editor.size()<40) editor+=c;
        else notice="Maximum 40 characters";
    } else if(screen==Screen::Message) {
        if(input.size()>=MaxMessage) { notice="Full: reset or use serial streaming"; return; }
        history_.push_back(machine.state()); input+=c; output+=machine.process(c); scroll=0; notice.clear();
    }
}
}

struct Size {
    int w, h;

    bool operator==(const Size &rhs) const {
        return this->w == rhs.w && this->h == rhs.h;
    }
};

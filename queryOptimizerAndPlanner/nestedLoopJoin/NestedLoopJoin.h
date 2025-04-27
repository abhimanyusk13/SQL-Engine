class NestedLoopJoin : public physical::PhysicalOperator {
    public:
        NestedLoopJoin(physical::PhysicalOperator *left,
                       physical::PhysicalOperator *right)
            : left_(left), right_(right), leftDone_(false) {}
        void open() override {
            left_->open();
            right_->open();
            // Prime first left row
            leftDone_ = !left_->next(leftRow_);
        }
        bool next(physical::Row &outRow) override {
            if (leftDone_) return false;
            // Try to get a right row
            physical::Row rightRow;
            while (true) {
                if (right_->next(rightRow)) {
                    // Emit concatenation
                    outRow = leftRow_;
                    outRow.insert(outRow.end(), rightRow.begin(), rightRow.end());
                    return true;
                }
                // Exhausted right, advance left
                right_->close();
                right_->open();
                if (!left_->next(leftRow_)) {
                    leftDone_ = true;
                    return false;
                }
            }
        }
        void close() override {
            left_->close();
            right_->close();
        }
    private:
        physical::PhysicalOperator *left_, *right_;
        physical::Row leftRow_;
        bool leftDone_;
    };
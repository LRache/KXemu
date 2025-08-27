$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(info + CXX $<)
	@ mkdir -p $(dir $@)
	@ $(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

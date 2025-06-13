1. Clone and Setup

  # Clone the repository
  git clone https://github.com/deepc0py/WarpDeck.git
  cd WarpDeck

  # Switch to the Phase 2 branch with all the fixes
  git checkout feature/phase2-cli

  # Verify you have the latest commits
  git log --oneline -3
  # Should show: 3c70b25 Fix critical initialization bugs in libwarpdeck

  2. Install Dependencies (if needed)

  # On Steam Deck (Arch Linux), you might need:
  sudo pacman -S cmake make gcc openssl avahi pkg-config

  # Or if using Flatpak dev environment:
  flatpak install org.freedesktop.Sdk//22.08

  3. Build libwarpdeck (Core Library)

  cd libwarpdeck
  mkdir -p build
  cd build
  cmake ..
  make -j$(nproc)
  cd ../..

  4. Build WarpDeck CLI

  cd warpdeck-cli
  mkdir -p build
  cd build
  cmake ..
  make -j$(nproc)

  5. Test the CLI

  # Should now work:
  ./warpdeck --help
  ./warpdeck config --set-name "Steam Deck"
  ./warpdeck list

#
#    This file is part of the Integrated Spectroscopic Framework (iSpec).
#    Copyright 2011-2012 Sergi Blanco Cuaresma - http://www.marblestation.com
#
#    iSpec is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Affero General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    iSpec is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Affero General Public License for more details.
#
#    You should have received a copy of the GNU Affero General Public License
#    along with iSpec. If not, see <http://www.gnu.org/licenses/>.
#
import logging
import logging.handlers

#LOG_LEVEL = "warning"
LOG_LEVEL = "info"
LOG_FILE = "ispec.log"
CONSOLE = True

logger = logging.getLogger() # root logger, common for all
#logger = logging.getLogger(name)
logger.setLevel(logging.getLevelName(LOG_LEVEL.upper()))

#formatter = logging.Formatter('[%(asctime)s] [%(levelname)s] %(name)s [%(funcName)s:%(lineno)d]: %(message)s')
formatter = logging.Formatter('[%(asctime)s] [%(levelname)s] [%(module)s:%(funcName)s:%(lineno)d]: %(message)s')
#formatter = logging.Formatter('[%(asctime)s] [%(levelname)s] %(name)s: %(message)s')

if CONSOLE:
    handler = logging.StreamHandler()
    handler.setFormatter(formatter)
    logger.addHandler(handler)

handler = logging.handlers.RotatingFileHandler(LOG_FILE, 'a', maxBytes=1048576, backupCount=5)
handler.setFormatter(formatter)
logger.addHandler(handler)
# It is accessible via import logging; logging.warn("x")